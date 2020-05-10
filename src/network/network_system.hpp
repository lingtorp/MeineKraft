#ifndef MEINEKRAFT_NETWORK_SYSTEM_HPP
#define MEINEKRAFT_NETWORK_SYSTEM_HPP

struct NetworkSystem {

  static NetworkSystem& instance() {
    static NetworkSystem system;
    return system;
  }



};

#endif // MEINEKRAFT_NETWORK_SYSTEM_HPP
