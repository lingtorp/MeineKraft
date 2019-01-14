#pragma once
#ifndef MEINEKRAFT_WORLD_HPP
#define MEINEKRAFT_WORLD_HPP

#include "../nodes/entity.h"
#include "../render/camera.h"
#include "../util/filesystem.h"
#include "../math/noise.h"

#include <array>
#include <set>
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <algorithm>

class Block: public Entity {
public:
  enum class BlockType {
    GRASS, DIRT
  };
  
  BlockType type;
  
  explicit Block(const Vec3f position, BlockType type): type(type) {
    NameSystem::instance().add_name_to_entity("Block", id);
    TransformComponent transform;
    transform.position = position;
    transform.scale = 1.0f;
    this->attach_component(transform);
    RenderComponent render_comp;
    render_comp.set_mesh(MeshPrimitive::Cube); 
    render_comp.set_cube_map_texture(textures_for_block(type));
    render_comp.set_shading_model(ShadingModel::Unlit); 
    this->attach_component(render_comp);
  }

  static std::vector<std::string> textures_for_block(BlockType type) {
    std::vector<std::string> faces;
    switch (type) {
    case BlockType::GRASS:
      faces = { Filesystem::base + std::string("resources/blocks/grass/side.jpg"),
                Filesystem::base + std::string("resources/blocks/grass/side.jpg"),
                Filesystem::base + std::string("resources/blocks/grass/top.jpg"),
                Filesystem::base + std::string("resources/blocks/grass/bottom.jpg"),
                Filesystem::base + std::string("resources/blocks/grass/side.jpg"),
                Filesystem::base + std::string("resources/blocks/grass/side.jpg") };
      break;
    case BlockType::DIRT:
      faces = { Filesystem::base + std::string("resources/blocks/dirt/bottom.jpg"),
                Filesystem::base + std::string("resources/blocks/dirt/bottom.jpg"),
                Filesystem::base + std::string("resources/blocks/dirt/bottom.jpg"),
                Filesystem::base + std::string("resources/blocks/dirt/bottom.jpg"),
                Filesystem::base + std::string("resources/blocks/dirt/bottom.jpg"),
                Filesystem::base + std::string("resources/blocks/dirt/bottom.jpg") };
      break;
    }
    return faces;
  }
};

class Chunk {
public:
  static const int dimension = 2;
  const Vec3i world_position;
  explicit Chunk(Vec3i world_position) : blocks{std::vector<std::vector<std::vector<Block>>>()}, world_position{world_position} {}

  const Block* block_at(const Vec3i& position) {
    if (position < Vec3i{dimension}) {
      return nullptr;
    }
    return &blocks[position.x][position.y][position.z];
  }
  
private:
  std::vector<std::vector<std::vector<Block>>> blocks;
};

struct World {
public:
  std::unordered_map<Vec3<int>, Chunk> chunks;
  
  explicit World(): chunks{} {
    std::mt19937 engine(1337);
    std::uniform_real_distribution<> distr(0.0, 1.0);

    std::vector<int> X(10);
    std::iota(X.begin(), X.end(), -10);
    for (const auto x : X) {
      Block::BlockType block_type = distr(engine) < 0.5 ? Block::BlockType::GRASS : Block::BlockType::DIRT;
      Block* block = new Block(Vec3f(0.0f, 0.0f, 1.0f + 1.0f * x), block_type);
    }

    Perlin noise(1337);
    const int32_t start = -11; // FIXME: Does not render correctly when larger than 11
    const int32_t end = -start;
    for (int32_t x = start; x < end; x++) {
      for (int32_t z = start; z < end; z++) {
        Block::BlockType block_type = distr(engine) < 0.5 ? Block::BlockType::GRASS : Block::BlockType::DIRT;
        Block* block = new Block(Vec3f(x, 0, z), block_type);

        const int32_t y_max = 20 * noise.fbm(Vec2d(x, z), 64);
        for (int32_t y = 1; y < y_max; y++) {
          Vec3f position = { Vec3f(x, y, z) };
          Block* block = new Block(position, block_type);
        }
      }
    }

    const size_t SIZE = 7;
    for (size_t i = 0; i < SIZE; i++) {
      for (size_t j = 0; j < SIZE; j++) {
        Entity* entity = new Entity();
        TransformComponent transform;
        transform.position = Vec3f{ 2.5f * j, 2.5f + 2.5f * i, -5.0f }; 
        entity->attach_component(transform);
        RenderComponent render;
        render.set_mesh(MeshPrimitive::Sphere);
        render.pbr_scalar_parameters = Vec3f(0.0, 1.0 / (double)(SIZE - 1) * i, (1.0 / (double)(SIZE - 1)) * j);
        render.set_shading_model(ShadingModel::PhysicallyBasedScalars);
        entity->attach_component(render);
        ActionComponent action([=](uint64_t frame, uint64_t dt) {
          Transform t = TransformSystem::instance().lookup(entity->id); 
          Vec3f position(transform.position.x, transform.position.y, 5.0f * std::cos(glm::radians(float(frame * 0.025f))));
          t.matrix = t.matrix.set_translation(position); // FIXME: Add translation, avoid copy
          TransformSystem::instance().set_transform(t, entity->id); 
        });
        entity->attach_component(action);
      }
    }
  }
  
  /// World position is measured in Chunk lengths
  Vec3f world_position(const Vec3f& position) const {
    Vec3f result{};
    result.x = std::round(position.x / Chunk::dimension) * Chunk::dimension;
    result.y = std::round(position.y / Chunk::dimension) * Chunk::dimension;
    result.z = std::round(position.z / Chunk::dimension) * Chunk::dimension;
    return result;
  }

  // TODO: Update all Entities 
  void tick() {

  }
};

#endif // MEINEKRAFT_WORLD_HPP
