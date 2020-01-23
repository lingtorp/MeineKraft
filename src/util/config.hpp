#pragma once

#include "../../include/json/json.hpp"

struct Scene;

/// Configuration of MeineKraft
/// Governed by config.json in root project folder
/// Please see documentation in docs/
struct Config {
  static nlohmann::json load_config(bool& success);
  static void save_scene(const Scene* scene);
};
