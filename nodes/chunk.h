#ifndef MEINEKRAFT_CHUNK_H
#define MEINEKRAFT_CHUNK_H

#include "../math/vector.h"
#include "../render/primitives.h"
#include "block.h"

class Chunk {
public:
    Chunk(Vec3<float> world_position);
    static const uint16_t dimension = 8; // The 'width' of the chunk in number of cubes
    Vec3<float> position;
    Vec3<float> center_position;
    uint16_t num_blocks;
    std::vector<std::unique_ptr<Block>> blocks;

    /// Transforms from world coordinates relative to Chunks coordinate system
    Vec2<float> world_coord(float x, float z);
};

#endif //MEINEKRAFT_CHUNK_H
