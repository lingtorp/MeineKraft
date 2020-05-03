#ifndef MEINEKRAFT_LOGGING_SYSTEM_H_
#define MEINEKRAFT_LOGGING_SYSTEM_H_

#include <string>
#include <array>
#include <mutex>
#include <vector>

struct LogMsg {
  std::string msg;
  uint16_t filter_flags;
};

struct LoggingSystem {
  std::mutex buffer_mutex;
  std::vector<LogMsg> buffer;
  uint16_t filter_flags;

  static LoggingSystem& instance() {
    static LoggingSystem instance;
    return instance;
  }

  void init() {
   
  }

  // Threadsafe callable on any thread - slow
  void add_msg(const std::string msg) {
    std::lock_guard<std::mutex> scoped(buffer_mutex);
    LogMsg logmsg;
    logmsg.msg = msg;
    buffer.push_back(logmsg);
  }

  // Callable only on main ImGui thread
  void draw_gui(bool* open);
};

#endif // MEINEKRAFT_LOGGING_SYSTEM_H_
