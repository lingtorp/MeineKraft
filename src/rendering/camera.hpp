#pragma once
#ifndef MEINEKRAFT_CAMERA_HPP
#define MEINEKRAFT_CAMERA_HPP

#include "../math/vector.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "../../include/json/json.hpp"

// TODO: Refactor the camera movement system

struct Camera {
  Camera() = default;
  Camera(nlohmann::json json);
  Camera(const Vec3f position, const Vec3f direction);

  /// Field of View in degrees
  float fov = 70.0f;

  /// Where x-axis is direction (forward from the camera) and the other axis are relative to it
  Vec3f direction = Vec3f::Z();

  /// Position of the camera
  Vec3f position = Vec3f::zero();

  /// Global positive up vector (y-axis)
  Vec3f up = Vec3f::Y();

  /// Pitch and yaw of the camera
  float pitch = 0.0f;
  float yaw = 0.0f;
  float roll = 0.0f; // FIXME: Unused for now

  /// Velocity in the three axis
  Vec3d velocity = Vec3d::zero();

  /// Max velocity of the movement along the three axis
  Vec3d max_velocity = Vec3d(5.0, 5.0, 5.0);

  /**
    * Acceleration indicates the direction of the axis that are moving
    * Acceleration: {x-axis{postive-direction, negative-direction}, ... }
    */
  Vec3<Vec2b> acceleration = {{false, false}, {false, false}, {false, false}};

  /// Update performs the acceleration calculations and returns the new position
  Vec3f update(uint32_t delta);

  // Start and stop the accelerations in the directions
  void move_forward(bool move);
  void move_backward(bool move);
  void move_right(bool move);
  void move_left(bool move);
  void move_down(bool move);
  void move_up(bool move);

  /// Calculates the a new direction vector based on the current pitch and yaw
  Vec3f recalculate_direction() const;

  /// Calculates the a new direction vector based on the current rotation
  glm::mat4 transform() const;

  /// Computes the projection matrix and returns it
  glm::mat4 projection(const float aspect) const;

  // TODO: Print more members
  friend std::ostream& operator<<(std::ostream& os, const Camera& cam) {
    return os << "Camera(position: " << cam.direction << ", direction: " << cam.direction << ")" << std::endl;
  }
};

#endif // MEINEKRAFT_CAMERA_HPP
