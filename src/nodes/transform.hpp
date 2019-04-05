#pragma once
#ifndef MEINEKRAFT_TRANSFORM_HPP
#define MEINEKRAFT_TRANSFORM_HPP

#include <unordered_map>

#include "../rendering/primitives.hpp"
#include "../math/quaternion.hpp"

struct NameSystem {
private:
  std::unordered_map<ID, std::string> data; // Raw data storage
public:
  /// Singleton instance of TransformSystem
  static NameSystem& instance() {
    static NameSystem instance;
    return instance;
  }

  void add_name_to_entity(const std::string& name, const ID id) {
    data[id] = name;
  }

  std::string get_name_from_entity(const ID id) {
    return data[id];
  }

  std::string* get_name_from_entity_referenced(const ID id) {
    return &data[id];
  }
};

struct TransformComponent {
  Vec3f position = Vec3f(0.0f, 0.0f, 0.0f); // World position
  float scale = 1.0f;
  Vec3f rotation = Vec3f(0.0f, 0.0f, 0.0f); // Rotation in degrees around (x, y, z)
};

// FIXME
inline static Mat4f compute_transform(const TransformComponent& comp) {
  return rotate(comp.rotation).translate(comp.position).scale(comp.scale);
}

struct TransformSystem {
private:
  std::vector<ID> data_ids;             // Entity ID for each Transform in data
  std::vector<TransformComponent> data; // Raw data storage
  std::unordered_map<ID, ID> data_idxs; // Entity ID to index into data
  size_t dirty_idx = 0;                 // Number of modified Transforms
public:
  /// Singleton instance of TransformSystem
  static TransformSystem& instance() {
    static TransformSystem instance;
    return instance;
  }

  void reset_dirty() {
    dirty_idx = 0;
  }

  std::vector<ID> get_dirty_transform_ids() const {
    std::vector<ID> dirty_ids(dirty_idx + 1);
    for (size_t i = 0; i <= dirty_idx; i++) {
      dirty_ids[i] = data_ids[i];
    }
    return dirty_ids;
  }

  /// Looking up with a non-existant ID returns the first element in the data, returns copy otherwise
  TransformComponent lookup(const ID id) {
    return data[data_idxs[id]]; 
  }

  /// Marks the data as dirty and returns a ptr to it
  TransformComponent* lookup_referenced(const ID id) {
    set_transform(data[data_idxs[id]], id); // Marks the data as dirty
    return &data[data_idxs[id]];
  }

  void set_transform(const TransformComponent& transform, const ID id) {
    if (data_idxs[id] <= dirty_idx) { // If transform is already dirty
      data[data_idxs[id]] = transform;
      return;
    }
    if (dirty_idx < data.size() - 1) {
      dirty_idx++;
    }
    if (data.size() >= 2) {
      data[data_idxs[id]] = data[dirty_idx]; // Move clean data to previous location
      data[dirty_idx] = transform;
      // Swap idxs
      const ID old_idx = data_idxs[id];
      data_idxs[id] = dirty_idx;
      data_idxs[data_ids[dirty_idx]] = old_idx;
      // Swap Entity IDs
      std::swap(data_ids[dirty_idx], data_ids[old_idx]);
    } 
  }

  void add_component(const TransformComponent& component, const ID id) {
    data.emplace_back(component);
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

#endif // MEINEKRAFT_TRANSFORM_HPP
