#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include <algorithm>
#include "../render/rendercomponent.h"

struct Camera;

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
    Vec3<float> position;
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
