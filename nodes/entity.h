#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include "../render/primitives.h"

class Camera;

class Entity {
public:
    uint64_t hash_id;
    Vec3<float> position;
    Vec3<float> rotation; // Rotation
    float scale;

    Entity(uint64_t hash_id): hash_id(hash_id), position{}, rotation{}, scale(1) {}

    virtual void update(uint64_t delta, const std::shared_ptr<Camera> camera) {};

    // TODO: The index system is fundamentally coupled with the Rendering which is not really great.
    // TODO: Might make more sense to bound it into the World or something ...
    /// To be overridden by subclasses that will provided a unique ID for each sub-class
    constexpr uint64_t generate_entity_id();
};

#endif //MEINEKRAFT_ENTITY_H
