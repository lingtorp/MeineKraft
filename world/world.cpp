#include <iostream>
#include <numeric>
#include "world.h"

World::World() {}

void World::world_tick(uint32_t delta, const std::shared_ptr<Camera> camera) {
    // Snap Camera/Player to the world coordinate grid
    auto camera_world_pos = world_position(camera->position);
    std::vector<GLfloat> x{camera_world_pos.x - Chunk::dimension, camera_world_pos.x, camera_world_pos.x + Chunk::dimension};
    std::vector<GLfloat> y{-Chunk::dimension};
    std::vector<GLfloat> z{camera_world_pos.z - Chunk::dimension, camera_world_pos.z, camera_world_pos.z + Chunk::dimension};
    std::vector<Vec3<>> positions = {};
    for (auto x : x) {
        for (auto y : y) {
            for (auto z : z) {
                auto position = Vec3<GLfloat>{x, y, z};
                positions.push_back(position);
                bool chunk_exists_at_pos = std::any_of(chunks.begin(), chunks.end(), [position](Chunk &c1){ return c1.position == position; });
                if (chunk_exists_at_pos) { continue; }
                chunks.push_back(Chunk::Chunk(position));
            }
        }
    }

    // TODO: Cull out chunks that are far away from the Player
    std::cout << "# of Chunk: " << chunks.size() << std::endl;
    if (chunks.size() > 12) {}
}

/// World position is measured in Chunk lengths
Vec3<> World::world_position(Vec3<> position) const {
    auto result = Vec3<>{};
    result.x = std::round(position.x / Chunk::dimension) * Chunk::dimension;
    result.y = std::round(position.y / Chunk::dimension) * Chunk::dimension;
    result.z = std::round(position.z / Chunk::dimension) * Chunk::dimension;
    return result;
}

/// Mainly used for testing noise functions
void World::spawn_flat_world() {
    static auto once = false;
    if (once) { return; } else { once = true; }
    /// Spawn a flat world once
    std::vector<GLfloat> x = {};
    std::vector<GLfloat> y = {-Chunk::dimension};
    std::vector<GLfloat> z = {};
    for (int i = -5; i < 5; i++) {
        x.push_back(i * Chunk::dimension);
        z.push_back(i * Chunk::dimension);
    }
    for (auto x : x) {
        for (auto y : y) {
            for (auto z : z) {
                auto position = Vec3<>{x, y, z};
                chunks.push_back(Chunk::Chunk(position));
            }
        }
    }
}