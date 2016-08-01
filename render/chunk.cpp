#include <cmath>
#include <iostream>
#include "chunk.h"

/// @param world_position World coordinates
Chunk::Chunk(Vec3 world_position):
        position(world_position), blocks{}, numCubes(0) {
    static auto noise = Noise();
    for (size_t x = 0; x < dimension; x++) {
        for (size_t z = 0; z < dimension; z++) {
            auto X = x + position.x;
            auto Z = z + position.z;
            auto height = std::round(noise.perlin(X, Z, world_position, dimension));
            std::cout << "Height: " << height;
            std::cout << " / Noise: " << noise.perlin(X, Z, world_position, dimension);
            std::cout << " for (x, z) = (" << X << ", " << Z << ")" << std::endl;
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