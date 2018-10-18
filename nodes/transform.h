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
  float scale = 1.0f;
};

static Transform compute_transform(const TransformComponent& comp) {
  return Transform(Mat4f().translate(comp.position).scale(comp.scale));
}

struct TransformSystem {
private:
  std::vector<Transform> data;
  std::unordered_map<ID, ID> data_idxs;
  size_t dirty_index = 0; // FIXME: Make use of it
public:
  /// Singleton instance of TransformSystem
  static TransformSystem& instance() {
    static TransformSystem instance;
    return instance;
  }

  std::vector<ID> get_dirty_transforms_from(const std::vector<ID>& ids) const {
    std::vector<ID> dirty_ids;
    for (const auto& id : ids) {
      if (data_idxs.find(id) == data_idxs.cend()) { continue; }
      if (data_idxs.at(id) >= dirty_index) {
        dirty_ids.emplace_back(id);
      }
    }
    return dirty_ids;
  }

  Transform lookup(const ID id) {
    if (data_idxs.find(id) == data_idxs.cend()) { return Transform(); }
    return data[data_idxs.at(id)]; 
  }

  void set_transform(const Transform& transform, const ID id) {
    data[data_idxs[id]] = transform;
  }

  void add_component(const TransformComponent& component, const ID id) {
    data.emplace_back(compute_transform(component));
    data_idxs[id] = data.size() - 1;
  }

  void remove_component(const ID id) {
    if (data_idxs.find(id) == data_idxs.cend()) { return; }
    data.erase(data.cbegin() + data_idxs[id]);
    data_idxs.erase(id);
  }
};

#endif // MEINEKRAFT_TRANSFORM_H