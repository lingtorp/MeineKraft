#pragma once

#include "../../include/json/json.hpp"

using json = nlohmann::json;

struct Scene;

struct Config {
  static json load_config();
  static void save_scene(const Scene* scene);
};
