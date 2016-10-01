#ifndef MEINEKRAFT_WORLD_H
#define MEINEKRAFT_WORLD_H

#include <stdint.h>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "../math/vector.h"
#include "../nodes/chunk.h"
#include "../render/camera.h"

class World {
public:
    World(uint64_t seed);

    std::vector<Chunk> chunks;

    void world_tick(uint32_t delta, std::shared_ptr<Camera> camera);
    Vec3<> world_position(Vec3<> position) const;

private:
    Noise noise;
    void spawn_flat_world();
};

#endif //MEINEKRAFT_WORLD_H
