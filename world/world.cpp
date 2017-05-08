#include "world.h"
#include "../render/camera.h"

World::World(uint64_t seed): noise(Noise(seed)) {}

/// Ticks the world
void World::world_tick(uint32_t delta, const std::shared_ptr<Camera> &camera) {
    for (auto entity : entities) {
        entity->update(delta, camera);
    }
}

void World::add_entity(Entity *entity) {
    entities.push_back(entity);
}
