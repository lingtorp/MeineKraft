#include "chunk.h"

/// @param world_position World coordinates
Chunk::Chunk(Vec3<> world_position, const Noise *noise):
        position(world_position), center_position{world_position.x + dimension/2,
                                                  world_position.y + dimension/2,
                                                  world_position.z + dimension/2},
                                                  blocks{}, numCubes(0) {
    static auto bottom = -32;
    for (size_t x = 0; x < dimension; x++) {
        for (size_t z = 0; z < dimension; z++) {
            auto X = x + position.x;
            auto Z = z + position.z;
            auto height = std::round(noise->octaves_of_perlin2d(X, Z, 2, 3, world_position, dimension));
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