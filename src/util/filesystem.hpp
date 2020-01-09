#pragma once
#ifndef MEINEKRAFT_FILESYSTEM_HPP
#define MEINEKRAFT_FILESYSTEM_HPP

#include <fstream>

// TODO: Make the paths dynamic rather than static
namespace Filesystem {
  /// Base filepath to the executeable
#if defined(__linux__)
  const std::string base = "/home/alexander/repos/MeineKraft-2/MeineKraft/";
#elif defined(__APPLE__)
  const std::string base = "/Users/lingtorp/repos/MeineKraft/";
#elif defined(_WIN32) || defined(_WIN64)
  const std::string base = "C:/Users/Alexander/repos/MeineKraft/";
#endif
  
#if defined(__linux__)
  const std::string home = "/home/alexander/";
#elif defined(__APPLE__)
  const std::string home = "/Users/lingtorp/";
#elif defined(_WIN32) || defined(_WIN64)
  const std::string home = "C:/Users/Alexander/";
#endif

  const std::string tmp = Filesystem::base + "tmp/";

  /// Check whether a file exists or not
  inline bool file_exists(const std::string& filename) {
    std::ifstream ifs(filename);
    return ifs.good();
  }

  /// Reads the entire contents of the file and returns it
  inline const std::string read_file(const std::string& filename) {
    std::ifstream ifs(filename); 
    return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
  }
}

#endif // MEINEKRAFT_FILESYSTEM_HPP
