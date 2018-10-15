#ifndef MEINEKRAFT_TRANSFORM_H
#define MEINEKRAFT_TRANSFORM_H

struct Transform {
  Mat4f matrix;
};

struct TransformComponent {
  Vec3f position;
  float scale;
  bool dirty_bit; // ?
};

struct TransformSystem {
private:
  std::vector<Transform> entities;
  std::vector<ID> lut;
public:
  /// Singleton instance of TransformSystem
  static TransformSystem& instance() {
    static TransformSystem instance;
    return instance;
  }

  void add_component(TransformComponent component, ID entity_id) {
    // TODO: Implement
  }

  void remove_component(ID entity_id) {
    // TODO: Implement
  }

  // Computes the transform of all the dirty TransformComponents
  void compute_transforms() {
    // Compute from dirty bit flag
    // Clear dirty flags
  }
};

#endif // MEINEKRAFT_TRANSFORM_H