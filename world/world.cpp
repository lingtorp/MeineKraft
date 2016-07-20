#include "world.h"

World::World() {
    std::vector<GLfloat> x{0};
    std::vector<GLfloat> y{0};
    std::vector<GLfloat> z{0};
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

World::~World() {}

void World::world_tick(uint32_t delta, std::shared_ptr<Camera> camera) {


}