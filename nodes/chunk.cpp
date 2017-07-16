#include "chunk.h"
#include <iostream>

Vec2<float> Chunk::world_coord(float x, float z) {
    /// Compress the coordinates inside the chunk; double part + int part = point coordinate
    float a = std::fmodf(x, dimension);     // Integer offset inside the chunk
    float xf = 1 - std::abs(a / dimension); // Float offset inside the chunk (0, 1)
    float xi = x / dimension;               // Integer bounds from the world
    float X = xf + xi;                      // Relative position inside the chunk

    float b = std::fmodf(z, dimension);
    float yf = 1 - std::abs(b / dimension);
    float yi = z / dimension;
    float Y = yf + yi;
    return {X, Y};
};

/// @param world_position World coordinates
Chunk::Chunk(Vec3<float> world_position):
        position(world_position), center_position{world_position.x + dimension/2,
                                                  world_position.y + dimension/2,
                                                  world_position.z + dimension/2},
                                                  blocks{}, num_blocks(0) {
//    static auto bottom = -32;
//    for (size_t x = 0; x < dimension; x++) {
//        for (size_t z = 0; z < dimension; z++) {
//            auto X = x + position.x;
//            auto Z = z + position.z;
//            auto coord = world_coord(X, Z);
//            auto height = std::round(noise.turbulence(coord.x, coord.y, 2, 3));
//            // std::cout << "Height: " << height;
//            // std::cout << " / Noise: " << noise.perlin(X, Z, world_position, dimension);
//            // std::cout << " for (x, z) = (" << X << ", " << Z << ")" << std::endl;
//            for (auto y = bottom; y < height; y++) {
//                auto block = std::make_unique<Block>();
//                block->position.x = x + position.x;
//                block->position.y = y + position.y;
//                block->position.z = z + position.z;
//                blocks.push_back(std::move(block));
//                num_blocks++;
//                /*
//                block->center = Vec3<float>{block->position.x + 0.5f,
//                                            block->position.y + 0.5f,
//                                            block->position.z + 0.5f};
//                block.radius = 1.0;
//                 */
//            }
//        }
//    }
};