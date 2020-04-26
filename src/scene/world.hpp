#pragma once
#ifndef MEINEKRAFT_WORLD_HPP
#define MEINEKRAFT_WORLD_HPP

#include "../rendering/rendercomponent.hpp"
#include "../nodes/entity.hpp"
#include "../nodes/physics_system.hpp"
#include "../rendering/camera.hpp"
#include "../util/filesystem.hpp"
#include "../math/noise.hpp"
#include "../nodes/transform.hpp"

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

  }

  Entity* spawn_entity(const MeshPrimitive mesh_primitive, const Vec3f position = Vec3f(), const Vec3f direction = Vec3f(), const float scale = 1.0f) {
    Entity* entity = new Entity();
    graph.push_back(entity);

    const std::string name = "Entity #" + std::to_string(entity->id);
    NameSystem::instance().add_name_to_entity(name, entity->id);

    TransformComponent transform;
    transform.position = position;
    transform.scale = scale;
    entity->attach_component(transform);

    auto t_post = TransformSystem::instance().lookup(entity->id);
    assert(t_post.position == transform.position);
    assert(t_post.scale == transform.scale);

    RenderComponent render;
    render.set_mesh(mesh_primitive);
    render.set_shading_model(ShadingModel::PhysicallyBasedScalars);
    const auto color = Vec3f(0.75f, 0.75f, 0.75f);
    render.set_emissive_color(color);
    render.set_diffuse_color(color);
    entity->attach_component(render);

    PhysicsComponent physics;
    physics.mass = 1.0f;
    physics.position = position;
    physics.velocity = direction.normalize();
    Log::info(direction.normalize());
    entity->attach_component(physics);

    return entity;
  }

  // TODO: Update all Entities
  void tick() {}
};

#endif // MEINEKRAFT_WORLD_HPP
