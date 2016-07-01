#include "world.h"

World::World() {
    for (size_t i = 0; i < MAX_CHUNKS; i++) {
        Vec3 chunk_pos = {0.0f, 0.0f, (GLfloat) i * 10};
        Chunk *chunk = World::new_chunk(chunk_pos);
        this->chunks[i] = chunk;
    }
}

World::~World() {}

void World::world_tick(double delta) {

}

Chunk *World::new_chunk(Vec3 position) {
    Chunk *chunk = (Chunk *) calloc(1, sizeof(Chunk));
    chunk->numCubes = BLOCKS_PER_CHUNK;
    GLfloat dim = cbrt(BLOCKS_PER_CHUNK);
    Vec3 chunk_dims = {dim, dim, dim};
    chunk->dimensions = chunk_dims;
    Color4 colors[] = {new_color_red(), new_color_green(), new_color_blue(),
                       new_color_yellow()};
    size_t index = 0;
    for (size_t i = 0; i < chunk->dimensions.x; i++) {
        for (size_t j = 0; j < chunk->dimensions.y; j++) {
            for (size_t k = 0; k < chunk->dimensions.z; k++) {
                Cube *cube = new_cube(colors);
                cube->position.x = i + position.x;
                cube->position.y = j + position.y;
                cube->position.z = k + position.z;
                chunk->blocks[index++] = *cube;
            }
        }
    }
    return chunk;
}