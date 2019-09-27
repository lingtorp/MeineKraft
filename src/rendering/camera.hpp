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

struct Camera {
  Camera(const Vec3f position, const Vec3f direction, const Vec3f up = Vec3f(0.0f, 1.0f, 0.0f));

  /// Where x-axis is direction (forward from the camera) and the other axis are relative to it
  Vec3f direction;

  /// Position of the camera
  Vec3f position;

  /// Global positive up vector (y-axis)
  Vec3f up;

  /// Pitch and yaw of the camera
  float pitch, yaw;

  /// Velocity in the three axis
  Vec3d velocity;

  /// Max velocity of the movement along the three axis
  const Vec3d max_velocity;

  /**
    * Acceleration indicates the direction of the axis that are moving
    * Acceleration: {x-axis{postive-direction, negative-direction}, ... }
    */
  Vec3<Vec2b> acceleration;

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

  /*
  /// Update performs the acceleration calculations and returns the new position
  Vec3<float> update(uint32_t delta);

  // Start and stop the accelerations in the directions
  void move_forward(bool move);
  void move_backward(bool move);
  void move_right(bool move);
  void move_left(bool move);
  void move_down(bool move);
  void move_up(bool move);

  /// Mouse diff vector from last frame
  Vec2<float> diff_vector;

  void move_forward();
  void move_left();
  void move_right();
  void move_backward();

  void update();
  */

  /// Calculates the a new direction vector based on the current rotation
  glm::mat4 transform() const;
};

#endif // MEINEKRAFT_CAMERA_HPP
