#ifndef MEINEKRAFT_WORLD_HPP
#define MEINEKRAFT_WORLD_HPP

#include "../nodes/entity.h"
#include "../render/camera.h"
#include "../util/filesystem.h"

#include <array>
#include <set>
#include <optional>
#include <unordered_map>
#include <cstdint>

class Block: public Entity {
  public:
  enum class BlockType {
    AIR, GRASS
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
    std::vector<std::string> faces = {Filesystem::base + std::string("resources/blocks/grass/side.jpg"),
                                      Filesystem::base + std::string("resources/blocks/grass/side.jpg"),
                                      Filesystem::base + std::string("resources/blocks/grass/top.jpg"),
                                      Filesystem::base + std::string("resources/blocks/grass/bottom.jpg"),
                                      Filesystem::base + std::string("resources/blocks/grass/side.jpg"),
                                      Filesystem::base + std::string("resources/blocks/grass/side.jpg")};
    return faces;
  }
};

class Chunk {
  public:
  static const int dimension = 8;
  const Vec3<int> world_position;
  explicit Chunk(Vec3<int> world_position):
          blocks{std::array<std::array<std::array<Block, dimension>, dimension>, dimension>()}, world_position{world_position} {}
  
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
  
  explicit World(const Camera* camera): chunks{} {
    /// Snap Camera/Player to the world coordinate grid
    auto camera_world_pos = world_position(camera->position);
    std::vector<float> x{camera_world_pos.x - Chunk::dimension, camera_world_pos.x, camera_world_pos.x + Chunk::dimension};
    std::vector<float> y{-Chunk::dimension};
    std::vector<float> z{camera_world_pos.z - Chunk::dimension, camera_world_pos.z, camera_world_pos.z + Chunk::dimension};
    for (auto x : x) {
      for (auto y : y) {
        for (auto z : z) {
          auto position = Vec3<int>{(int)x, (int)y, (int)z};
          if (chunks.count(position) == 0) {
            chunks.emplace(position, Chunk{position});
          }
        }
      }
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
