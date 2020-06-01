#pragma once
#ifndef MEINEKRAFT_FILESYSTEM_HPP
#define MEINEKRAFT_FILESYSTEM_HPP

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <string>
#include <regex>
#include <cassert>

#include "../rendering/texture.hpp"

// TODO: Make the paths dynamic rather than static
namespace Filesystem {
  /// Base filepath to the executeable root folder
#if defined(__linux__)
  const std::string base = "/home/alexander/repos/MeineKraft/";
#elif defined(_WIN32) || defined(_WIN64)
  const std::string base = "C:/Users/Alexander/repos/MeineKraft/";
#endif

#if defined(__linux__)
  const std::string home = "/home/alexander/";
#elif defined(_WIN32) || defined(_WIN64)
  const std::string home = "C:/Users/Alexander/";
#endif

  const std::string tmp = Filesystem::base + "tmp/";

  /// Check whether a file exists or not
  static bool file_exists(const std::string& filename) {
    std::ifstream ifs(filename);
    return ifs.good();
  }

  /// Reads the entire contents of the file and returns it
  static const std::string read_file(const std::string& filename) {
    std::ifstream ifs(filename); 
    return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
  }

  /// Tries to create the directory if it is not already present
  static void create_directory(const std::string& filepath) {
    std::filesystem::create_directory(filepath);
  }

  static std::string save_image_as_ppm(const std::string filename, const float* pixels, const size_t w, const size_t h, const float downsample_factor = 1.0f, const TextureFormat fmt = TextureFormat::RGB32F);

  static std::string save_image_as(const std::string filename, const ImageFormat img_fmt, const float* pixels, const size_t w, const size_t h, const float downsample_factor = 1.0f, const TextureFormat texture_fmt = TextureFormat::RGB32F);

  /// Saves the text passed in a file at filepath
  static void save_text_in_file(const std::string& filepath, const std::string& txt) {
    std::ofstream file;
    file.open(filepath + ".txt");
    if (file.is_open()) {
      file << txt;
    }
  }
}
#endif // MEINEKRAFT_FILESYSTEM_HPP
