#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include <algorithm>
#include "../render/rendercomponent.h"

struct Camera;

/*
using Entity = ID; // ... ?

enum class ComponentFlag: uint32_t {
  GraphicsState = 1 << 0,
  Name          = 1 << 1,
  Position      = 1 << 2
};

struct ComponentData {
  std::vector<std::string> names;
  std::vector<Vec3f> positions;
  std::vector<float> scales;
  std::vector<Mat4f> transforms;
  std::vector<GraphicsState> graphics_states;
  std::vector<ComponentFlag> components;
};

// Mapping: Entity ID <--> Components
struct EntitySystem {
  ComponentData objects;

  EntitySystem(): objects{} {

  }

  // Map entity ID to the right bitflag
};

struct TransformSystem {
  struct TransformEntity {
    Entity entity;
    bool dirty_bit;
  };
  std::vector<TransformEntity> entities;

  void add_entity(Entity entity) {

  }

  void translate(Entity entity, Vec3f translation) {
    // Mark Entity as dirty

  }

  // Computes the transform of all the dirt Entities
  void update() {
    // Sort entities based on dirty bit
    // Compute
    // Clear dirty flags
  }
};
*/

class Entity {
private:
    std::vector<Component*> components;

    /// Generates a new Entity id to be used when identifying this instance of Entity
    ID generate_entity_id() const {
        static uint64_t e_id = 0;
        return e_id++;
    };

public:
    ID entity_id;
    Vec3f position;
    float scale;

    Entity(): entity_id(generate_entity_id()), position{}, scale(1.0f) {}
    ~Entity() {
      for (auto& comp : components) { comp->did_deattach_from_entity(this); }
    }

    virtual void update(const uint64_t delta, const Camera& camera) {};

    /** Component handling **/
    void attach_component(Component* comp) {
        comp->did_attach_to_entity(this);
        components.push_back(comp);
    }

    void deattach_component(Component* comp) {
        comp->did_deattach_from_entity(this);
        components.erase(std::remove(components.begin(), components.end(), comp), components.end());
    }
};

#endif // MEINEKRAFT_ENTITY_H
