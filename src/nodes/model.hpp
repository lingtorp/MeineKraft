#pragma once
#ifndef MEINEKRAFT_MODEL_HPP
#define MEINEKRAFT_MODEL_HPP

#include "../rendering/camera.hpp"
#include "entity.hpp"
#include "../rendering/rendercomponent.hpp"

class Model: public Entity {
public:
  Model(const std::string& directory, const std::string& file);
  Model(const RenderComponent& render);
};

struct Scene {
	AABB aabb;
  Camera* camera;
  Scene(const std::string& directory, const std::string& file);
};

#endif // MEINEKRAFT_MODEL_HPP
