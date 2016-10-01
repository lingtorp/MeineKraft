#ifndef MEINEKRAFT_CHUNK_H
#define MEINEKRAFT_CHUNK_H

#include "../math/vector.h"
#include "../render/primitives.h"
#include "../math/noise.h"
#include <cmath>
#include <iostream>

class Chunk {
public:
    Chunk(Vec3<> world_position, const Noise *noise);
    static const uint16_t dimension = 32; // The 'width' of the chunk in number of cubes
    Vec3<> position;
    Vec3<> center_position;
    uint16_t numCubes;
    std::vector<Cube> blocks;
    bool will_be_removed;
};

#endif //MEINEKRAFT_CHUNK_H
