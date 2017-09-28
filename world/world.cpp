#include "world.h"
#include "../render/camera.h"

World::World(uint64_t seed): noise(new Perlin(seed)) {
  /// Spawn terrain
  size_t max_altitude = 4;
  size_t width = 2;
  size_t height = 2;
  for (size_t x = 0; x < width; x++) {
  for (size_t y = 0; y < height; y++) {
      double noise_value = noise->turbulence(x, y, 32);
      int32_t altitude = (int32_t) std::ceil(noise_value * max_altitude);
      for (size_t z = 0; z != altitude; altitude < 0 ? z-- : z++) {
        Block *block = new Block();
        block->position = Vec3<float>{(float) x, (float) z, (float) y};
      }
    }
  }
}

/// Ticks the world
void World::world_tick(uint32_t delta, const std::shared_ptr<Camera> &camera) {
  for (auto entity : entities) {
    entity->update(delta, camera);
  }
}

void World::add_entity(Entity *entity) {
  entities.push_back(entity);
}
