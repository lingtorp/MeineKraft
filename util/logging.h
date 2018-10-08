#ifndef MEINEKRAFT_LOGGING_H
#define MEINEKRAFT_LOGGING_H

#include <iostream>

namespace Log {
  template<typename T>
  static inline void log_to_console(const T& msg, const std::string& type, 
                                    const char* file, const int line) {
#ifdef LOGGING_VERBOSE
    std::cerr << type << " (" << file << ", " << line << "):" << msg << std::endl; 
#else 
    std::cerr << type << ": " << msg << std::endl;
#endif
  }

  #define error(msg) log_to_console(msg, "ERROR", __FILE__, __LINE__)
  #define warn(msg)  log_to_console(msg, "WARNING", __FILE__, __LINE__)
  #define info(msg)  log_to_console(msg, "INFO", __FILE__, __LINE__)
} 

#endif // MEINEKRAFT_LOGGING_H