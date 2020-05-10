#pragma once
#ifndef MEINEKRAFT_LOGGING_HPP
#define MEINEKRAFT_LOGGING_HPP

#include <iostream>
#include <sstream>
#include <vector>

#define DEBUG

#include "logging_system.hpp"

// Logging
inline bool logging_to_console = true;
inline bool logging_to_editor = true;
inline bool logging_verbose = false;

namespace Log {
  template<typename T>
  static inline void log(const size_t indent, const T& msg, const std::string& type,
                         const char* file, const int line, const bool new_line = true) {
    std::stringstream str;

    if (logging_verbose) {
      str << type << " (" << file << ", " << line << "):";
    } else {
      str << type << ": ";
    }

    for (size_t i = 0; i < indent; i++) {
      str << "\t";
    }
    str << msg;
    if (new_line) {
      str << std::endl;
    }

    const std::string string = str.str();

    if (logging_to_console) {
      std::cerr << string;
    }

    if (logging_to_editor) {
      LoggingSystem::instance().add_msg(str.str());
    }
  }

  // Template specialization over std::vector<T>
  template<typename T>
  static inline void log(const size_t indent, const std::vector<T>&msg, const std::string& type,
                         const char* file, const int line) {
    std::stringstream str;

    if (logging_verbose) {
      str << type << " (" << file << ", " << line << "):";
    } else {
      str << type << ": ";
    }

    for (size_t i = 0; i < indent; i++) {
      str << "\t";
    }
    str << msg;

    for (const auto& item : msg) {
      str << item << ", ";
    }
    str << "]" << std::endl;

    const std::string string = str.str();

    if (logging_to_console) {
      std::cerr << string << std::endl;
    }

    if (logging_to_editor) {
      LoggingSystem::instance().add_msg(string);
    }

  }

  // Non-indented versions
  #define error(msg) log(0, msg, "ERROR", __FILE__, __LINE__)
  #define warn(msg)  log(0, msg, "WARNING", __FILE__, __LINE__)
  #define info(msg)  log(0, msg, "INFO", __FILE__, __LINE__)

  // Indented versions
  #define info_indent(indent, msg) log(indent, msg, "INFO", __FILE__, __LINE__)

  // Debug printouts are no operations when DEBUG is disabled
#ifdef DEBUG
  #define dbg(msg) log(0, msg, "DEBUG", __FILE__, __LINE__)
#else
  #define dbg(msg)
#endif

}

#endif // MEINEKRAFT_LOGGING_HPP
