#ifndef MEINEKRAFT_WORLD_H
#define MEINEKRAFT_WORLD_H

#include <stdint.h>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "../math/vector.h"
#include "../render/chunk.h"
#include "../render/camera.h"

class World {
public:
    World();
    ~World();

    std::vector<Chunk> chunks;

    void world_tick(uint32_t delta, std::shared_ptr<Camera> camera);
};

#endif //MEINEKRAFT_WORLD_H
