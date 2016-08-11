#include <cmath>
#include <iostream>
#include "chunk.h"

/// @param world_position World coordinates
Chunk::Chunk(Vec3<> world_position):
        position(world_position), blocks{}, numCubes(0) {
    static auto noise = Noise();
    static auto bottom = -10;
    for (size_t x = 0; x < dimension; x++) {
        for (size_t z = 0; z < dimension; z++) {
            auto X = x + position.x;
            auto Z = z + position.z;
 //           auto height = std::round(std::sqrt(noise.perlin(X, Z, world_position, dimension)));
            auto height = noise.generate(X, Z);
            // std::cout << "Height: " << height;
            // std::cout << " / Noise: " << noise.perlin(X, Z, world_position, dimension);
            // std::cout << " for (x, z) = (" << X << ", " << Z << ")" << std::endl;
            for (auto y = bottom; y < height; y++) {
                Cube cube = Cube();
                cube.position.x = x + position.x;
                cube.position.y = y + position.y;
                cube.position.z = z + position.z;
                cube.center = Vec3<>{(GLfloat) (cube.position.x + 0.5),
                                     (GLfloat) (cube.position.y + 0.5),
                                     (GLfloat) (cube.position.z + 0.5)};
                cube.radius = 1.0;
                blocks.push_back(cube);
                numCubes++;
            }
        }
    }
};