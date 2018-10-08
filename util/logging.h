#ifndef MEINEKRAFT_LOGGING_H
#define MEINEKRAFT_LOGGING_H

#include <string>
#include <iostream>

namespace Log {
    static inline void log_to_console(const std::string& msg, const std::string& type, 
                                      const char* file, const int line) {
        std::cerr << type << "(" << file << ", " << line << "):" << msg << std::endl; 
    }

    #define error(msg) log_to_console(msg, "ERROR", __FILE__, __LINE__)
    #define warn(msg)  log_to_console(msg, "WARNING", __FILE__, __LINE__)
    #define info(msg)  log_to_console(msg, "INFO", __FILE__, __LINE__)
} 

#endif // MEINEKRAFT_LOGGING_H