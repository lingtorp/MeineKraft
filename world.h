#ifndef MEINEKRAFT_WORLD_H
#define MEINEKRAFT_WORLD_H

#include <stdint.h>
#include "math/vector.h"
#include "render/primitives.h"

static const uint16_t BLOCKS_PER_CHUNK = 1000; // Cubic root must be an integer
struct Chunk {
    Vec3 position;
    uint16_t numCubes;
    Vec3 dimensions;
    Cube blocks[BLOCKS_PER_CHUNK];
};

class World {
public:
    World();

    ~World();

    static const uint8_t MAX_CHUNKS = 9; // Max # of chunks in the world
    Chunk *chunks[MAX_CHUNKS];

    void world_tick(double delta);

    Chunk *new_chunk(Vec3 position);
};

#endif //MEINEKRAFT_WORLD_H
