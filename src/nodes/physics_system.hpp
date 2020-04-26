#pragma once
#include "transform.hpp"
#ifndef MEINEKRAFT_PHYSICS_SYSTEM_HPP
#define MEINEKRAFT_PHYSICS_SYSTEM_HPP

#include <unordered_map>

#include "entity.hpp"
#include "../rendering/primitives.hpp"
#include "../util/logging.hpp"

// Contains information needed to perform some simple physics and collision detection computations
struct PhysicsComponent {
  Vec3f velocity = Vec3f(0.0f);
  float mass = 0.0f;
  // Proxy geometry is a sphere for broad phase collision detection
  Vec3f position = Vec3f(0.0f); // Modifications to transforms does not affect physics because of this
  float radius = 0.0f;
};

static bool collision_detection(PhysicsComponent& a, PhysicsComponent& b) {
  return (a.position - b.position).length() <= (a.radius + b.radius);
};

static void collision_resolution(PhysicsComponent& a, PhysicsComponent& b) {
  const Vec3f a_to_b = (b.position - a.position).normalize();
  a.velocity = -a_to_b;
  b.velocity = a_to_b;
};

struct PhysicsSystem {
  std::unordered_map<ID, ID> mapping;
  std::vector<PhysicsComponent> comps;

  /// Singleton instance
  static PhysicsSystem& instance() {
    static PhysicsSystem instance;
    return instance;
  }

  // Compute changes since last frame
  void update_system(const int64_t dt_ms) {
    // Collision detection
    // for (PhysicsComponent& a : comps) {
    //   for (PhysicsComponent& b : comps) {
    //     if (collision_detection(a, b)) {
    //         collision_resolution(a, b);
    //     }
    //   }
    // }

    // Compute and modify in place
    for (const auto [id, idx] : mapping) {
      TransformComponent* transform = TransformSystem::instance().lookup_referenced(id);
      PhysicsComponent& comp = comps[idx];
      transform->position += 0.1f * comp.velocity * dt_ms;

      // Boxed ...
      if (transform->position.x < 200.0f) {
        comp.velocity.x = -comp.velocity.x;
        transform->position.x = 200.0f;
      }

      if (transform->position.x > 800.0f) {
        comp.velocity.x = -comp.velocity.x;
        transform->position.x = 800.0f;
      }

      if (transform->position.y < 200.0f) {
        comp.velocity.y = -comp.velocity.y;
        transform->position.y = 200.0f;
      }

      if (transform->position.y > 600.0f) {
        comp.velocity.y = -comp.velocity.y;
        transform->position.y = 600.0f;
      }

      if (transform->position.z < -200.0f) {
        comp.velocity.z = -comp.velocity.z;
        transform->position.z = -200.0f;
      }

      if (transform->position.z > 200.0f) {
        comp.velocity.z = -comp.velocity.z;
        transform->position.z = 200.0f;
      }

    }
  }

  void add_component(const PhysicsComponent& comp, const ID id) {
    comps.push_back(comp);
    const auto idx = comps.size() - 1;
    mapping[id] = idx;
  }

  PhysicsComponent& lookup(const ID id) {
    return comps[mapping[id]];
  }
};

#endif // MEINEKRAFT_PHYSICS_SYSTEM_HPP
