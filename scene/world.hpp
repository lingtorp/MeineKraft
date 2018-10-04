#ifndef MEINEKRAFT_WORLD_HPP
#define MEINEKRAFT_WORLD_HPP

#include "../nodes/entity.h"
#include "../render/camera.h"
#include "../util/filesystem.h"

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
    AIR, GRASS, DIRT
  };
  
  BlockType type;
  
  explicit Block(BlockType type = BlockType::AIR): type(type) {
    if (type == BlockType::AIR) { return; }
    auto render_comp = new RenderComponent(this);
    render_comp->set_mesh(MeshPrimitive::Cube);
    render_comp->set_cube_map_texture(textures_for_block(type));
    attach_component(render_comp);
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
  const Vec3<int> world_position;
  explicit Chunk(Vec3<int> world_position): blocks{std::array<std::array<std::array<Block, dimension>, dimension>, dimension>()}, world_position{world_position} {}
  
  const Block* block_at(Vec3<int> position) {
    if (position < Vec3<int>{dimension}) {
      return nullptr;
    }
    return &blocks[position.x][position.y][position.z];
  }
  
  private:
  std::array<std::array<std::array<Block, dimension>, dimension>, dimension> blocks;
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
      Block* block = new Block(block_type);
      block->position = Vec3<float>(0, 0, x);
      std::cerr << block->position << std::endl;
    }
  }
  
  /// World position is measured in Chunk lengths
  Vec3<float> world_position(Vec3<float> position) const {
    Vec3<float> result{};
    result.x = std::round(position.x / Chunk::dimension) * Chunk::dimension;
    result.y = std::round(position.y / Chunk::dimension) * Chunk::dimension;
    result.z = std::round(position.z / Chunk::dimension) * Chunk::dimension;
    return result;
  }
};

#endif // MEINEKRAFT_WORLD_HPP
