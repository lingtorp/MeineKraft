#ifndef MEINEKRAFT_FILESYSTEM_H
#define MEINEKRAFT_FILESYSTEM_H

namespace Filesystem {
  // TODO: Make these dynamic rather than static
  /// Base filepath to the executeable
  #ifdef __linux__
  const std::string base = "/home/lingtorp/repos/MeineKraft/";
  #elif __APPLE__
  const std::string base = "/Users/lingtorp/repos/MeineKraft/";
  #elif defined(_WIN32) || defined(_WIN64)
  const std::string base = "/Users/lingtorp/repos/MeineKraft/";
  #endif
}

#endif // MEINEKRAFT_FILESYSTEM_H
