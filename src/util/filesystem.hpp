#pragma once
#ifndef MEINEKRAFT_FILESYSTEM_HPP
#define MEINEKRAFT_FILESYSTEM_HPP

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <string>
#include <regex>

#include "logging.hpp"
#include "../math/vector.hpp"

// TODO: Make the paths dynamic rather than static
namespace Filesystem {
  /// Base filepath to the executeable root folder
#if defined(__linux__)
  const std::string base = "/home/alexander/repos/MeineKraft-2/MeineKraft/";
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
  inline bool file_exists(const std::string& filename) {
    std::ifstream ifs(filename);
    return ifs.good();
  }

  /// Reads the entire contents of the file and returns it
  inline const std::string read_file(const std::string& filename) {
    std::ifstream ifs(filename); 
    return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
  }

  /// Tries to create the directory if it is not already present
  inline void create_directory(const std::string& filepath) {
    std::filesystem::create_directory(filepath);
  }

  /// Tries to save the pixels as RGB PPM format
  /// Returns filepath to the saved file if it was created successfully, otherwise empty string
  inline std::string save_image_as_ppm(const std::string filename, const Vec3f* pixels, const size_t w, const size_t h, const float downsample_factor = 1.0f) {
    assert(downsample_factor > 1.0f && "Downsample factor must be larger or equal to 1.0");

    std::filesystem::path filepath = filename;

    const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string timestamp = std::ctime(&time);
    timestamp.erase(std::remove(timestamp.begin(), timestamp.end(), '\n'), timestamp.end()); // Trim \n
    filepath += "-" + std::regex_replace(timestamp, std::regex(" "), "-"); // Replace whitespace with '-'

    filepath += ".ppm";

    if (w == 0 || h == 0) {
      Log::warn("Tried save screenshot with width or height of zero.");
      return "";
    }

    if (!pixels) {
      Log::warn("Tried to save screenshot with no pixels.");
      return "";
    }

    if (filepath.empty()) {
      Log::warn("Tried to save screenshot with empty filepath.");
      return "";
    }

    if (std::filesystem::is_directory(filepath)) {
      Log::warn("Tried to save screenshot with a directoy filepath: " + filepath.string());
      return "";
    }

    const size_t width  = static_cast<size_t>(w / downsample_factor);
    const size_t height = static_cast<size_t>(h / downsample_factor);

    std::ofstream file(filepath);
    file << "P3 \n";                                                        // ASCII RGB magic number
    file << std::to_string(width) << " " << std::to_string(height) << "\n"; // Image dimension in pixels
    file << "255 \n";                                                       // Maximum color channel value
    file << "# Image generated by MeineKraft rendering engine \n";

    // NOTE: OpenGL origin is lower left so flip the image
    const int32_t starty = (h - 1) / downsample_factor;
    const int32_t stopy  = 0;
    const uint32_t startx = 0;
    const uint32_t stopx  = w / downsample_factor;
    assert(stopx > startx);
    for (int32_t y = starty; y >= stopy; y--) {
      for (uint32_t x = startx; x < stopx; x++) {
        const Vec3f& p = pixels[y * w + x];
        file << std::to_string(static_cast<uint8_t>(p.x * 255.0f)) + " ";
        file << std::to_string(static_cast<uint8_t>(p.y * 255.0f)) + " ";
        file << std::to_string(static_cast<uint8_t>(p.z * 255.0f)) + " ";
      }
    }

    file.close();

    Log::info("Screenshot saved at: " + filepath.string());
    return filepath;
  }

  /// Saves the text passed in a file at filepath
  inline void save_text_in_file(const std::string& filepath, const std::string& txt) {
    std::ofstream file;
    file.open(filepath + ".txt");
    if (file.is_open()) {
      file << txt;
    }
  }
}
#endif // MEINEKRAFT_FILESYSTEM_HPP
