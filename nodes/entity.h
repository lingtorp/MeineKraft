#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include <algorithm>
#include "../render/rendercomponent.h"
#include "transform.h"
#include "../render/render.h"

#include <functional>
struct ActionComponent {
  std::function<void(uint64_t, uint64_t)> action;
  ActionComponent(const std::function<void(uint64_t, uint64_t)>& action): action(action) {}
};

struct ActionSystem {
  ActionSystem() {}
  ~ActionSystem() {}
  /// Singleton instance
  static ActionSystem& instance() {
    static ActionSystem instance;
    return instance;
  }

  std::vector<ActionComponent> components;

  void add_component(const ActionComponent& component) {
    components.emplace_back(component);
  }

  void execute_actions(const uint64_t frame, const uint64_t dt) {
    for (const auto& component : components) {
      component.action(frame, dt);
    }
  }
};

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

  /// Lookup if the Entity is alive
  bool lookup(ID entity) const {

  }

  // Map entity ID to the right bitflag
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
      Renderer::instance().add_component(component, id);
    }

    void attach_component(const TransformComponent component) {
      TransformSystem::instance().add_component(component, id);
    }

    void attach_component(const ActionComponent component) {
      ActionSystem::instance().add_component(component);
    }

    void deattach_component(const RenderComponent component) {
      Renderer::instance().remove_component(id);
    }

    void deattach_component(const TransformComponent component) {
      TransformSystem::instance().remove_component(id);
    }
};

#endif // MEINEKRAFT_ENTITY_H
