#include "config.hpp"

#include "../nodes/model.hpp"
#include "filesystem.hpp"
#include "logging.hpp"

#include <filesystem>
#include <iomanip>

using json = nlohmann::json;

// NOTE: Disabled until GCC-9 is released as default compiler on Ubuntu
json Config::load_config() {
  const std::string json_str = Filesystem::read_file(Filesystem::base + "config.json");
  json config = json::parse(json_str);

  // Camera
  if (Filesystem::file_exists(Filesystem::tmp + "camera.json")) {
    const std::string json_str = Filesystem::read_file(Filesystem::tmp + "camera.json");
    const json camera_config = json::parse(json_str);
    config.merge_patch(camera_config);
  }

  return config;
}

void Config::save_scene(const Scene *scene) {
  return; // NOTE: Disabled until GCC-9 is released as default compiler on Ubuntu
  // Log::info("Saving scene ...");

  // const std::filesystem::path tmp_path("/home/alexander/repos/Meinekraft/tmp/");
  // // const std::filesystem::path tmp_path(Filesystem::tmp); // FIXME: This actually crashes (something in the STL deallocs something it should not I believe)
  // if (!std::filesystem::is_directory(tmp_path)) {
  //   std::error_code err;
  //   const bool success = std::filesystem::create_directory(tmp_path, err);
  //   if (!success) {
  //     Log::error("\t Could not create /tmp directory, skipping saving the Scene ...");
  //     Log::error("\t \t Error message: " + err.message());
  //     return;
  //   } 
  // }

  // // Camera
  // {
  //   std::ofstream file(tmp_path.string() + "camera.json");

  //   json j =
  //   { {"scene",
  //      {{"camera",
  //        {{"position",
  //          {scene->camera->position.x, scene->camera->position.y,
  //           scene->camera->position.z}},
  //         {"direction",
  //          {scene->camera->direction.x, scene->camera->direction.y,
  //           scene->camera->direction.z}}}}}} };

  //   file << std::setw(4) << j << std::endl;
  // }

  // Log::info("✓ Scene successfully saved!");
}
