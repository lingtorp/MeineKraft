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
  return Transform(Mat4f().translate(comp.position).scale(comp.scale).transpose());
}

struct TransformSystem {
private:
  std::vector<Transform> data;
  std::unordered_map<ID, ID> data_idxs;
public:
  /// Singleton instance of TransformSystem
  static TransformSystem& instance() {
    static TransformSystem instance;
    return instance;
  }

  Transform lookup(const ID id) {
    return data[data_idxs[id]];
  }

  void add_component(TransformComponent component, ID id) {
    data.push_back(compute_transform(component));
    data_idxs[id] = data.size() - 1;
  }

  void remove_component(ID id) {}
};

#endif // MEINEKRAFT_TRANSFORM_H