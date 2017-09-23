#ifndef MEINEKRAFT_WORLD_H
#define MEINEKRAFT_WORLD_H

#include <stdint.h>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include "../nodes/chunk.h"
#include "../math/noise.h"

class Entity;
class Camera;

class World {
public:
    World(uint64_t seed);

    std::vector<Entity *> entities;
    std::unordered_map<Vec3<float>, std::shared_ptr<Chunk>> chunks;

    void world_tick(uint32_t delta, const std::shared_ptr<Camera> &camera);
    void add_entity(Entity *entity);

private:
    Noise *noise;
};

#endif //MEINEKRAFT_WORLD_H
