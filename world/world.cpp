#include <iostream>
#include <numeric>
#include "world.h"

World::World() {
    std::vector<GLfloat> x(2);
    std::vector<GLfloat> y{-Chunk::dimension, -Chunk::dimension};
    std::vector<GLfloat> z(2);
    std::iota(x.begin(), x.end(), -1);
    std::iota(z.begin(), z.end(), -1);
    for (auto x : x) {
        for (auto y : y) {
            for (auto z : z) {
                Vec3 chunk_pos = {x, y, z};
                auto chunk = Chunk::Chunk(chunk_pos);
                chunks.push_back(chunk);
           }
        }
    }
}

void World::world_tick(uint32_t delta, const std::shared_ptr<Camera> camera) {
    // FIXME: The world floor is overlapping and thus removing the previous chunks it looks like.
    // 1. Snap Camera/Player to the world coordinate grid using it's direction vector.
    auto camera_world_pos = Vec3{std::round(camera->position.x + camera->direction.x), std::round(camera->position.y + camera->direction.y), std::round(camera->position.z + camera->direction.z)};
    std::vector<GLfloat> x{camera_world_pos.x - Chunk::dimension, camera_world_pos.x, camera_world_pos.x + Chunk::dimension};
    std::vector<GLfloat> y{-Chunk::dimension};
    std::vector<GLfloat> z{camera_world_pos.z - Chunk::dimension, camera_world_pos.z, camera_world_pos.z + Chunk::dimension};
    for (auto x : x) {
        if ((int) x % Chunk::dimension != 0) { continue; }
        for (auto y : y) {
            for (auto z : z) {
                if ((int) y % Chunk::dimension != 0) { continue; }
                Vec3 position = {x, y, z};
                bool chunk_exists_at_pos = std::any_of(chunks.begin(), chunks.end(), [position](Chunk &c1){ return c1.position == position; });
                if (chunk_exists_at_pos) { continue; }
                chunks.push_back(Chunk::Chunk(position));
            }
        }
    }
}
