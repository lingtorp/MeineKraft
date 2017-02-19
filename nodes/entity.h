#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include "../render/primitives.h"
#include "../render/rendercomponent.h"

class Camera;

class Entity {
private:
    // Does an Entity own it's components?
    std::vector<Component *> components;

    /// Generates a new Entity id to be used when identifying this instance of Entity
    uint64_t generate_entity_id() const {
        static uint64_t e_id = 0;
        return e_id++;
    };

public:
    uint64_t entity_id;

    Vec3<float> position;
    Vec3<float> rotation;
    float scale;
    Vec3<float> center;
    double radius;

    Entity(): entity_id(generate_entity_id()), position{}, rotation{}, scale(1), center{}, radius(0) {}
    ~Entity();

    /// Called once every frame
    virtual void update(uint64_t delta, const std::shared_ptr<Camera> camera) {};

    /** Component handling **/
    void attach_component(Component *comp) {
        comp->did_attach_to_entity(this);
        components.push_back(comp);
    }

    void deattach_component(Component *comp) {
        comp->did_deattach_from_entity(this);
        components.erase(std::remove(components.begin(), components.end(), comp), components.end());
    }
};

#endif //MEINEKRAFT_ENTITY_H
