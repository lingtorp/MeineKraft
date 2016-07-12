#include <numeric>
#include <vector>
#include "world.h"

World::World(): chunks() {
    std::vector<GLfloat> x(1);
    std::vector<GLfloat> y(1);
    std::vector<GLfloat> z(1);
    std::iota(x.begin(), x.end(), 1);
    std::iota(y.begin(), y.end(), 1);
    std::iota(z.begin(), z.end(), 1);
    for (auto x : x) {
        for (auto y : y) {
            for (auto z : z) {
                Vec3 chunk_pos = {x, y, z};
                Chunk *chunk = World::new_chunk(chunk_pos);
                this->chunks.push_back(chunk);
            }
        }
    }
}

World::~World() {}

void World::world_tick(double delta) { }

Chunk *World::new_chunk(Vec3 position) {
    Chunk *chunk = (Chunk *) calloc(1, sizeof(Chunk));
    chunk->numCubes = BLOCKS_PER_CHUNK;
    GLfloat dim = cbrt(BLOCKS_PER_CHUNK);
    Vec3 chunk_dims = {dim, dim, dim};
    chunk->dimensions = chunk_dims;
    Color4 colors[] = {Color4::red(), Color4::green(), Color4::blue(),
                       Color4::yellow()};
    size_t index = 0;
    for (size_t i = 0; i < chunk->dimensions.x; i++) {
        for (size_t j = 0; j < chunk->dimensions.y; j++) {
            for (size_t k = 0; k < chunk->dimensions.z; k++) {
                Cube cube = Cube(colors);
                cube.position.x = i + position.x;
                cube.position.y = j + position.y;
                cube.position.z = k + position.z;
                chunk->blocks[index++] = std::move(cube);
            }
        }
    }
    return chunk;
}