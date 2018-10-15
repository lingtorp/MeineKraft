#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include <algorithm>
#include "../render/rendercomponent.h"
#include "../render/render.h"

struct Camera;

// Mapping: Entity ID <--> Alive?
struct EntitySystem {
private:
  std::vector<ID> entities;
  std::unordered_map<ID, ID> lut;

public:
  EntitySystem() {}
  ~EntitySystem() {}
  /// Singleton instance
  static EntitySystem& instance() {
    static EntitySystem instance;
    return instance;
  }
  
  /// Generates a new Entity id to be used when identifying this instance of Entity
  ID new_entity() const {
    // TODO: Implement something for real
    static uint64_t e_id = 0;
    return e_id++;
  };

  // Map entity ID to the right bitflag
  // lookup ... alive??
  void destroy_entity(const ID& id) {
    // TODO: Implement by removing all the components owned by the Entity (again this is mainly a convenicence thing)
  }
};

/// Game object 
struct Entity {
    ID id;

    Entity(): id(EntitySystem::instance().new_entity()) {}
    ~Entity() {
      EntitySystem::instance().destroy_entity(id);
    }

    /** Component handling for convenience **/
    void attach_component(const RenderComponent component) {
        Renderer::instance().add_to_batch(component, id);
    }

    void deattach_component(const RenderComponent component) {
        // TODO: Example: Renderer::instance().remove_from_batch(id);
    }
};

struct Transform {
  Vec3f position;
  float scale;
  bool dirty_bit; // ?
};

struct TransformSystem {
private:
  std::vector<Transform> entities;
  std::vector<ID> lut;
public:

  void add_entity(ID id) {
    // TODO: Implement
  }

  // Computes the transform of all the dirt Entities
  void update() {
    // Compute from dirty bit flag
    // Clear dirty flags
  }
};

#endif // MEINEKRAFT_ENTITY_H
