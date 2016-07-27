#include <cmath>
#include <iostream>
#include "chunk.h"
#include "../math/noise.h"

/// @param world_position World coordinates
Chunk::Chunk(Vec3 world_position):
        position(world_position), blocks{}, numCubes(0) {
    for (size_t x = 0; x < dimension; x++) {
        for (size_t z = 0; z < dimension; z++) {
            auto height = std::ceil(noise::perlin(x, z) * 10);
            std::cout << "Height: " << height << std::endl;
            for (size_t y = 0; y < height; y++) {
                Cube cube = Cube();
                cube.position.x = x + position.x;
                cube.position.y = y + position.y;
                cube.position.z = z + position.z;
                blocks.push_back(cube);
                numCubes++;
            }
        }
    }
};