#ifndef MEINEKRAFT_MKASS_HPP
#define MEINEKRAFT_MKASS_HPP

#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cctype>
#include <stdint.h>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <cassert>

#include "logging.hpp"

/// Production rules for the
/// <string>:
/// <instruction> ...
/// <instruction> = addi | jump | label | call
/// addi <register> <int>
/// jump | call <string>
/// <register> = r0 | r1 | ... | rN

/// Runtime context of the MkAss language
/// Provides as small virtual machine context for execution
struct MkAssContext {
  bool pause = false; // Pauses execution at the next instruction
  bool exit = false;  // Set to true to stop execution
  size_t pc = 0;
  std::array<size_t, 4> regs  = {0}; // Registers
  std::array<size_t, 4> stack = {0}; // Stack
  size_t sp = 0;                     // Stack pointer (current top of the stack)
  bool compare_flag = false;
  std::unordered_map<std::string, std::function<void(MkAssContext&)>> external_symbol_table = {};

  // Registers a handler for the external symbol used in the context
  // 1. Register handlers for external symbols
  // 2. Called with the runtime context at the call site
  void register_external_symbol(std::string label, std::function<void(MkAssContext&)> handler) {
    std::transform(label.begin(), label.end(), label.begin(), [](unsigned char c){ return std::tolower(c); });
    external_symbol_table[label] = handler;
  }
};

/// MkAss (yes 'make ASS')
/// Provides utility to handle MkLang and run it
namespace MkAss {
  struct Token {
    enum class Type: size_t {
      addi = 0, call = 1, jump = 2, label = 3, cmpi = 4, brneq = 5, ret = 6
    };

    enum Token::Type type;
    union {
      int32_t args[2];    // Immediate instructions (addi, etc)
      struct {
        int32_t addr;     // Label addr (jump) (-1 = invalid/not resolved), (call) (-1 = external call using label.name for lookup)
        const char* name; // Label name (label), resolved after tokenization to addr
      } label;
      void* fn;           // Function ptr (call)
    };

    friend std::ostream& operator<<(std::ostream& os, const Token& t) {
      std::string type_str = "Token{type: ";
      std::string data_str = "";
      switch (t.type) {
        case Token::Type::addi:
          type_str += "addi";
          data_str = "r" + std::to_string(t.args[0]) + ", imm.:" + std::to_string(t.args[1]);
          break;
        case Token::Type::call:
          type_str += "call";
          data_str = "label: " + std::string(t.label.name) + "(" + "addr: " + std::to_string(t.label.addr) + ")";
          break;
        case Token::Type::jump:
          type_str += "jump";
          data_str = "label: " + std::string(t.label.name) + "(" + "addr: " + std::to_string(t.label.addr) + ")";
          break;
        case Token::Type::label:
          type_str += "label";
          data_str = "label: " + std::string(t.label.name) + "(" + "addr: " + std::to_string(t.label.addr) + ")";
          break;
        case Token::Type::brneq:
          type_str += "label";
          data_str = "label: " + std::string(t.label.name) + "(" + "addr: " + std::to_string(t.label.addr) + ")";
          break;
        case Token::Type::cmpi:
          type_str += "cmpi";
          data_str = "r" + std::to_string(t.args[0]) + ", imm.:" + std::to_string(t.args[1]);
          break;
        case Token::Type::ret:
          type_str += "ret";
          break;
        default: Log::error("Undefined instruction"); assert(false);
      }
      return os << type_str << ", " << data_str << "}";
    }
  };

  std::vector<MkAss::Token> tokenize(MkAssContext& ctx, std::string src) {
    std::vector<MkAss::Token> tokens = {};
    size_t num_tokens = 0;

    std::transform(src.begin(), src.end(), src.begin(), [](unsigned char c){ return std::tolower(c); });
   
    // Tokenize
    constexpr char delimiters[] = " \n\t";
    char* tks = strtok(&src[0], delimiters);

    std::vector<char*> token_strs;
    while (tks != nullptr) {
      token_strs.push_back(tks);
      num_tokens++;
      Log::dbg(tks);
      tks = strtok(nullptr, delimiters);
    }

    // Syntax: token stream -> expressions (N <= M)
    for (size_t i = 0; i < num_tokens; i++) {
      Log::info("----" + std::string(token_strs[i]) + "-----");
      char* s = token_strs[i];
      const size_t s_lng = strlen(s);

      // TODO: Handle comments

      // Label statements
      if (s[s_lng - 1] == ':') {
        s[s_lng - 1] = '\0'; // FIXME: Ugly hack
        Log::info("Label: " + std::string(s));

        Token token;
        token.type = Token::Type::label;
        token.label.name = s;
        token.label.addr = -1;
        Log::info(s);
        tokens.push_back(token);
      }
      // Other tokens
      else if (strcmp(s, "addi") == 0) {
        const char* arg0 = token_strs[++i];
        const char* arg1 = token_strs[++i];
        Log::info(std::string(s) + " + " + std::string(arg0) + " + " + std::string(arg1));

        Token token;
        token.type = Token::Type::addi;
        token.args[0] = std::atoi(arg0); // TODO: handle rX
        assert(0 <= token.args[0] && token.args[0] < 4);
        token.args[1] = std::atoi(arg1);
        tokens.push_back(token);
      }
      else if (strcmp(s, "call") == 0) {
        const char* arg0 = token_strs[++i];
        Log::info(std::string(s) + " + " + arg0);

        Token token;
        token.type = Token::Type::call;
        token.label.name = arg0;
        token.label.addr = -1;
        tokens.push_back(token);
      }
      else if (strcmp(s, "jump") == 0) {
        const char* arg0 = token_strs[++i];
        Log::info(std::string(s) + " + " + arg0);

        Token token;
        token.type = Token::Type::jump;
        token.label.name = arg0;
        token.label.addr = -1;
        tokens.push_back(token);
      }
      else if (strcmp(s, "cmpi") == 0) {
        const char* arg0 = token_strs[++i];
        const char* arg1 = token_strs[++i];
        Log::info(std::string(s) + " + " + std::string(arg0) + " + " + std::string(arg1));

        Token token;
        token.type = Token::Type::cmpi;
        token.args[0] = std::atoi(arg0);
        token.args[1] = std::atoi(arg1);
        tokens.push_back(token);
      }
      else if (strcmp(s, "brneq") == 0) {
        const char* arg0 = token_strs[++i];
        Log::info(std::string(s) + " + " + arg0);

        Token token;
        token.type = Token::Type::brneq;
        token.label.name = arg0;
        tokens.push_back(token);
      }
      else if (strcmp(s, "ret") == 0) {
        Log::info(std::string(s));

        Token token;
        token.type = Token::Type::ret;
        tokens.push_back(token);
      }
      else {
        Log::warn("Unrecognized statement: " + std::string(s));
        return {};
      }
    }

    // Resolve symbols in labels and calls
    std::unordered_map<std::string, size_t> symbol_table;
    for (size_t i = 0; i < tokens.size(); i++) {
      if (tokens[i].type == Token::Type::label) {
        if (symbol_table.count(tokens[i].label.name) == 0) {
          symbol_table[tokens[i].label.name] = i;
        } else {
          Log::error("Multiple labels with same name defined.");
        }
      }
    }

    for (size_t i = 0; i < tokens.size(); i++) {
      if (tokens[i].type == Token::Type::call || tokens[i].type == Token::Type::jump || tokens[i].type == Token::Type::label || tokens[i].type == Token::Type::brneq) {
        if (symbol_table.count(tokens[i].label.name) == 0) {

        // Resolve external symbols of call instructions
        if (tokens[i].type == Token::Type::call) {
          if (ctx.external_symbol_table.count(tokens[i].label.name) == 0) {
            Log::error("Undefined external symbol: " + std::string(tokens[i].label.name)); // FIXME: Improve err msg
          }
        } else {
          Log::error("Undefined label used: " + std::string(tokens[i].label.name));
        }

        } else {
          tokens[i].label.addr = symbol_table[tokens[i].label.name];
        }
      }
    }

    //

    for (size_t i = 0; i < tokens.size(); i++) {
      Log::dbg(tokens[i]);
    }

    //

    return tokens;
  }

  void run(MkAssContext& ctx, const std::string& src) {
    const std::vector<MkAss::Token> tokens = MkAss::tokenize(ctx, src);
    if (tokens.size() == 0) {
      return;
    }

    while (!ctx.exit) {
      if (ctx.pause) { continue; }

      const Token& token = tokens[ctx.pc];

      switch (token.type) {
        case MkAss::Token::Type::jump:
          ctx.pc = token.label.addr;
          break;
        case MkAss::Token::Type::label:
          /* Do nothing */
          break;
        case MkAss::Token::Type::addi:
          ctx.regs[token.args[0]] += token.args[1];
          break;
        case MkAss::Token::Type::call:
          if (token.label.addr == -1) {
            // External call
            if (ctx.external_symbol_table.count(token.label.name) == 0) {
              Log::error("[MkRuntime]: Runtime failure. External symbol " + std::string(token.label.name) + " not registered");
            } else {
              ctx.external_symbol_table[token.label.name](ctx);
            }
          } else {
            // Interal call
            ctx.stack[ctx.sp++] = ctx.pc;
            ctx.pc = token.label.addr;
          }
          break;
        case MkAss::Token::Type::cmpi:
          ctx.compare_flag = (ctx.regs[token.args[0]] == token.args[1]);
          break;
        case MkAss::Token::Type::brneq:
          if (!ctx.compare_flag) {
            ctx.pc = token.label.addr;
          }
          break;
        case MkAss::Token::Type::ret:
          ctx.pc = ctx.stack[--ctx.sp];
          break;
        default: assert(false);
      }

      ctx.pc++;
    }
  }
};

#include "imgui/imgui.h"

struct MkAssProgramManager {
  struct MkAssRuntimeContext {
    const std::string src;
    MkAssContext runtime_ctx;
  };
 
  std::vector<MkAssContext> ctxs;

  static MkAssProgramManager& instance() {
    static MkAssProgramManager obj;
    return obj;
  }

  bool init() {
    return true;
  }

  void draw_gui() {
    for (size_t c = 0; c < ctxs.size(); c++) {
      MkAssContext& ctx = ctxs[c];

      for (size_t i = 0; i < ctx.regs.size(); i++) {
        ImGui::Text("R%lu: %lu \t", i, ctx.regs[i]); ImGui::SameLine();
      }
    }
  }

  MkAssContext& new_ctx() {
    MkAssContext ctx;
    ctxs.push_back(ctx);
    return ctxs.back();
  }

  void run(MkAssContext& ctx, std::string src) {
    MkAss::run(ctx, src);
  }

  void step() {
    // TODO: ?
  }
};



#endif // MEINEKRAFT_MKASS_HPP
