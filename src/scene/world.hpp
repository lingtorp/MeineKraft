#pragma once
#ifndef MEINEKRAFT_WORLD_HPP
#define MEINEKRAFT_WORLD_HPP

#include "../nodes/entity.hpp"
#include "../rendering/camera.hpp"
#include "../util/filesystem.hpp"
#include "../math/noise.hpp"

#include <array>
#include <set>
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <algorithm>

struct World {
  std::vector<Entity*> graph;
 
  explicit World() {
    spawn_entity(MeshPrimitive::Sphere, Vec3f(800.0f, 200.0f, 0.0f), 60.0f);
  }

  Entity* spawn_entity(const MeshPrimitive mesh_primitive, const Vec3f position = Vec3f(), const float scale = 1.0f) {
    Entity* entity = new Entity();
    graph.push_back(entity);

    const std::string name = "Entity #" + std::to_string(entity->id);
    NameSystem::instance().add_name_to_entity(name, entity->id);

    TransformComponent transform;
    transform.position = position;
    transform.scale = scale;
    entity->attach_component(transform);

    RenderComponent render;
    render.set_mesh(mesh_primitive);
    render.set_shading_model(ShadingModel::PhysicallyBasedScalars);
    const auto color = Vec3f(0.75f, 0.75f, 0.75f);
    render.set_emissive_color(color);
    render.set_diffuse_color(color);
    entity->attach_component(render);

    return entity;
  }
  // TODO: Update all Entities
  void tick() {}
};

#endif // MEINEKRAFT_WORLD_HPP
