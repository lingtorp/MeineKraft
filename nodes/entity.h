#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include "../render/primitives.h"

class Entity {
public:
    uint64_t hash_id;
    Vec3<float> position;
    float theta_x, theta_y, theta_z; // Rotation

    Entity(): hash_id(get_next_hash_id(0)), position{}, theta_x(0), theta_y(0), theta_z(0) {}

private: // Every Entity gets a new hash_id, should be every new type of subclass instead
    constexpr uint64_t get_next_hash_id(uint64_t old_hash_id) {
        return old_hash_id + 1;
    }
};

#endif //MEINEKRAFT_ENTITY_H
