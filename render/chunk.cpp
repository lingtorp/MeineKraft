#include "chunk.h"

/// @param world_position World coordinates
Chunk::Chunk(Vec3 world_position):
        position(world_position), numCubes(BLOCKS_PER_CHUNK), blocks(BLOCKS_PER_CHUNK) {
    GLfloat dim = cbrt(BLOCKS_PER_CHUNK);
    Vec3 chunk_dims = {dim, dim, dim};
    dimensions = chunk_dims;

    size_t index = 0;
    for (size_t i = 0; i < dimensions.x; i++) {
        for (size_t j = 0; j < dimensions.y; j++) {
            for (size_t k = 0; k < dimensions.z; k++) {
                Cube cube = Cube();
                cube.position.x = i + position.x;
                cube.position.y = j + position.y;
                cube.position.z = k + position.z;
                blocks[index++] = cube;
            }
        }
    }
};