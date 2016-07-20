#ifndef MEINEKRAFT_CHUNK_H
#define MEINEKRAFT_CHUNK_H

#include "../math/vector.h"
#include "primitives.h"

class Chunk {
public:
    Chunk(Vec3 world_position);

    static const uint16_t BLOCKS_PER_CHUNK = 125; // Cubic root must be an integer
    Vec3 position;
    uint16_t numCubes;
    Vec3 dimensions;
    std::vector<Cube> blocks;
};

#endif //MEINEKRAFT_CHUNK_H
