#pragma once
#ifndef MEINEKRAFT_LOGGING_HPP
#define MEINEKRAFT_LOGGING_HPP

#include <iostream>
#include <vector>

#define DEBUG

namespace Log {
  template<typename T>
  static inline void log_to_console(const size_t indent, const T& msg, const std::string& type, 
                                    const char* file, const int line, const bool new_line = true) {
#ifdef LOGGING_VERBOSE
    std::cerr << type << " (" << file << ", " << line << "):";
#else 
    std::cerr << type << ": ";
#endif
    for (size_t i = 0; i < indent; i++) {
      std::cerr << "\t";
    }
    std::cerr << msg;
    if (new_line) {
      std::cerr << std::endl;
    }
  }

  // Template specialization over std::vector<T>
  template<typename T>
  static inline void log_to_console(const size_t indent, const std::vector<T>&msg, const std::string& type,
                                    const char* file, const int line) {
    log_to_console(indent, "[", type, file, line, false);
    for (const auto& item : msg) {
      std::cerr << item << ", ";
    }
    std::cerr << "]" << std::endl;
  }

  // Non-indented versions
  #define error(msg) log_to_console(0, msg, "ERROR", __FILE__, __LINE__)
  #define warn(msg)  log_to_console(0, msg, "WARNING", __FILE__, __LINE__)
  #define info(msg)  log_to_console(0, msg, "INFO", __FILE__, __LINE__)

  // Debug printouts are no operations when DEBUG is disabled
#ifdef DEBUG
  #define dbg(msg)   log_to_console(0, msg, "DEBUG", __FILE__, __LINE__)
#else
  #define dbg(msg)
#endif

  // Indented versions
  #define info_indent(indent, msg) log_to_console(indent, msg, "INFO", __FILE__, __LINE__)
} 

#endif // MEINEKRAFT_LOGGING_HPP
