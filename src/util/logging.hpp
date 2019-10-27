#pragma once
#ifndef MEINEKRAFT_LOGGING_HPP
#define MEINEKRAFT_LOGGING_HPP

#include <iostream>

namespace Log {
  template<typename T>
  static inline void log_to_console(const size_t indent, const T& msg, const std::string& type, 
                                    const char* file, const int line) {
#ifdef LOGGING_VERBOSE
    std::cerr << type << " (" << file << ", " << line << "):";
#else 
    std::cerr << type << ": ";
#endif
    for (size_t i = 0; i < indent; i++) {
      std::cerr << "\t";
    }
    std::cerr << msg << std::endl;
  }

  // Non-indented versions
  #define error(msg) log_to_console(0, msg, "ERROR", __FILE__, __LINE__)
  #define warn(msg)  log_to_console(0, msg, "WARNING", __FILE__, __LINE__)
  #define info(msg)  log_to_console(0, msg, "INFO", __FILE__, __LINE__)

  // Indented versions
  #define info_indent(indent, msg) log_to_console(indent, msg, "INFO", __FILE__, __LINE__)
} 

#endif // MEINEKRAFT_LOGGING_HPP
