#pragma once
#ifndef MEINEKRAFT_MODEL_HPP
#define MEINEKRAFT_MODEL_HPP

#include "../rendering/camera.hpp"
#include "../rendering/rendercomponent.hpp"
#include "entity.hpp"

class Model: public Entity {
public:
  Model(const std::string& directory, const std::string& file);
  Model(const RenderComponent& render);
};

struct Scene {
  /// Scene-wide AABB containing all Models
  AABB aabb;

  /// Scene main camera
  Camera camera;

  /// Scene directional light
  DirectionalLight directional_light = DirectionalLight(Vec3f(0.0f, -1.0f, -0.3f));

  Scene() = default;
  Scene(const std::string& directory, const std::string& file);

  /// Loads all models from the following file into the Scene
  void load_models_from(const std::string& directory, const std::string& file);

  /// Moves and positions the camera as it was spawned
  void reset_camera();  
};

#endif // MEINEKRAFT_MODEL_HPP
