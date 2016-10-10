#include "world.h"
#include "../render/camera.h"
#include "../nodes/entity.h"
#include "../nodes/chunk.h"

World::World(uint64_t seed): noise(Noise(seed)) {}

/// Ticks the world
void World::world_tick(uint32_t delta, const std::shared_ptr<Camera> camera) {
    for (auto entity : entities) {
        entity->update(delta, camera);
    }

    // TODO: Improve or replace the solution below ...
    // Snap Camera/Player to the world coordinate grid
    auto camera_world_pos = world_position(camera->position);
    std::vector<float> x{camera_world_pos.x - Chunk::dimension, camera_world_pos.x, camera_world_pos.x + Chunk::dimension};
    std::vector<float> y{-Chunk::dimension};
    std::vector<float> z{camera_world_pos.z - Chunk::dimension, camera_world_pos.z, camera_world_pos.z + Chunk::dimension};
    for (auto x : x) {
        for (auto y : y) {
            for (auto z : z) {
                auto position = Vec3<float>{x, y, z};
                bool chunk_exists_at_pos = std::any_of(chunks.begin(), chunks.end(), [position](std::shared_ptr<Chunk> c1){ return c1->position == position; });
                if (chunk_exists_at_pos) { continue; }
                auto chunk = std::make_shared<Chunk>(position, noise);
                chunks.push_back(chunk);
            }
        }
    }

    // TODO: Cull out entities by inactivating those that are far away from the Player
    // TODO: Also the inverse of above
}

/// World position is measured in Chunk lengths
Vec3<float> World::world_position(Vec3<float> position) const {
    Vec3<float> result{};
    result.x = std::round(position.x / Chunk::dimension) * Chunk::dimension;
    result.y = std::round(position.y / Chunk::dimension) * Chunk::dimension;
    result.z = std::round(position.z / Chunk::dimension) * Chunk::dimension;
    return result;
}

void World::add_entity(Entity *entity) {
    entities.push_back(entity);
}
