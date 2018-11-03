#ifndef MEINEKRAFT_TRANSFORM_H
#define MEINEKRAFT_TRANSFORM_H

#include <unordered_map>
#include "../render/primitives.h"

struct Transform {
  Mat4f matrix;
  Transform() = default;
  Transform(const Mat4f& mat): matrix(mat) {}
};

struct TransformComponent {
  Vec3f position = Vec3f(0.0f, 0.0f, 0.0f);
  // TODO: Rotation
  float scale = 1.0f;
};

static Transform compute_transform(const TransformComponent& comp) {
  return Transform(Mat4f().translate(comp.position).scale(comp.scale));
}

struct TransformSystem {
private:
  /*
  Use as something like 
  uint32_t INDEX_MASK = 0x0000FFFF;
  in order to create a key-value pair in the data_idxs to support reverse lookup from the data position
  index to the Entity ID in the data_idxs 
  uint32_t ENTITY_MASK = 0xFFFF0000;
  uint32_t index = id & ENTITY_MASK;
  index += data_index_of_entity & INDEX_MASK;
  
  */
  std::vector<ID> data_ids;
  std::vector<Transform> data;
  std::unordered_map<ID, ID> data_idxs;
  size_t dirty_idx = 0; 
public:
  /// Singleton instance of TransformSystem
  static TransformSystem& instance() {
    static TransformSystem instance;
    return instance;
  }

  void reset_dirty() {
    dirty_idx = 0;
  }

  std::vector<ID> get_dirty_transforms_from(const std::vector<ID>& ids) const {
    std::vector<ID> dirty_ids;
    for (const auto& id : ids) {
      if (data_idxs.at(id) <= dirty_idx) {
        dirty_ids.emplace_back(id);
      }
    }
    /*
    for (const auto& id : ids) {
      if (data_idxs.find(id) == data_idxs.cend()) { continue; }
      dirty_ids.emplace_back(id);
    }
    */
    return dirty_ids;
  }

  /// Looking up with a non-existant ID returns the first element in the data
  Transform lookup(const ID id) {
    return data[data_idxs[id]]; 
  }

  void set_transform(const Transform& transform, const ID id) {
    // data[data_idxs[id]] = transform;
    if (dirty_idx < data.size() - 1) {
      dirty_idx++;
    }
    if (data.size() >= 2) {
      const Transform temp = data[dirty_idx];
      data[dirty_idx] = transform;
      data[data_idxs[id]] = temp;
      // Swap idxs
      const ID old_idx = data_idxs[id];
      data_idxs[id] = dirty_idx;
      data_idxs[data_ids[dirty_idx]] = old_idx;
      // Swap 
      std::swap(data_ids[dirty_idx], data_ids[old_idx]);
    } 
  }

  void add_component(const TransformComponent& component, const ID id) {
    data.emplace_back(compute_transform(component));
    data_idxs[id] = data.size() - 1;
    data_ids.emplace_back(id);
  }

  void remove_component(const ID id) {
    if (data_idxs.find(id) == data_idxs.cend()) { return; }
    data.erase(data.cbegin() + data_idxs[id]);
    data_idxs.erase(id);
    // resize data?
  }
};

#endif // MEINEKRAFT_TRANSFORM_H