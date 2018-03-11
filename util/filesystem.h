#ifndef MEINEKRAFT_FILESYSTEM_H
#define MEINEKRAFT_FILESYSTEM_H

namespace Filesystem {
  // TODO: Make these dynamic rather than static
  /// Base filepath to the executeable
  #ifdef __linux__
  const std::string base = "/home/alexander/repos/MeineKraft/";
  #elif __APPLE__
  const std::string base = "/Users/AlexanderLingtorp/Repositories/MeineKraft/";
  #endif
}

#endif // MEINEKRAFT_FILESYSTEM_H
