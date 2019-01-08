#include "camera.h"

#include <algorithm>
#include "../math/quaternion.h"
#include "../render/primitives.h"

/*
Camera::Camera(Vec3<float> position, Vec3<float> direction, Vec3<float> up):
        position(position), direction(direction.normalize()), up(up.normalize()) {};

void Camera::move_forward() {
  position = position + direction * 0.05f;
}

void Camera::move_left() {
  position = position + direction.cross(up).normalize() * - 0.05f;
}

void Camera::move_right() {
  position = position + direction.cross(up).normalize() * 0.05f;
}

void Camera::move_backward() {
  position = position + direction * -0.05f;
}

void Camera::update() {
  const float sensitivity = 1.0f;
  const float dx = glm::radians(diff_vector.x * sensitivity);
  const float dy = glm::radians(diff_vector.y * sensitivity);

  quat rotation(direction.cross(up));
  direction = rotation.rotate(direction, dy).normalize();
  up = rotation.rotate(up, dy).normalize();

  quat yrotation(up);
  direction = yrotation.rotate(direction, dx).normalize();
}
*/

/// Clamps a number to between lo and hi, in other words: [lo, hi]
static double clamp(const double x, const double lo, const double hi) {
  return std::min(std::max(x, lo), hi);
}

Camera::Camera(Vec3<float> position, Vec3<float> direction, Vec3<float> world_up) :
  direction(direction), position(position), up(world_up),
  pitch(0), yaw(0), velocity{ 0.0, 0.0, 0.0 }, max_velocity{ 0.5, 0.5, 0.5 }, acceleration{ {false, false}, {false, false}, {false, false} } {};

void Camera::move_forward(bool move) {
  acceleration.x.x = move;
}

void Camera::move_backward(bool move) {
  acceleration.x.y = move;
}

void Camera::move_right(bool move) {
  acceleration.z.x = move;
}

void Camera::move_left(bool move) {
  acceleration.z.y = move;
}

void Camera::move_down(bool move) {
  acceleration.y.x = move;
}

void Camera::move_up(bool move) {
  acceleration.y.y = move;
}

Vec3<float> Camera::recalculate_direction() const {
  const bool euler_angles = true;
  if (euler_angles) {
    static constexpr float rad = PI / 180.0f;
    Vec3<float> result;
    result.x = -sinf(yaw * rad) * cosf(pitch * rad);
    result.y = sinf(pitch * rad);
    result.z = -cosf(yaw * rad) * cosf(pitch * rad);
    return result.normalize();
  } else {
    const float sensitivity = 0.2f;
    const float dx = glm::radians(yaw * sensitivity);
    const float dy = glm::radians(pitch * sensitivity);

    quat rotation(direction.cross(up));
    Vec3<float> new_direction = rotation.rotate(direction, dy).normalize();
    Vec3<float> new_up = rotation.rotate(up, dy).normalize();

    quat yrotation(new_up);
    return yrotation.rotate(new_direction, dx).normalize();
  }
}

Vec3<float> Camera::update(const uint32_t delta) {
  /// Perform acceleration and de-acceleration calculations for each direction on each axis.
  if (acceleration.x.x) {
    // If accelerating along positive x-axis, increase the velocity along the positive axis
    velocity.x = clamp(velocity.x + 0.01 * delta / 16.0, 0.0, max_velocity.x);
  }
  else if (acceleration.x.y) {
    // If accelerating along negative x-axis, increase the velocity along the negative axis
    velocity.x = clamp(velocity.x - 0.01 * delta / 16.0, -1.0, 0.0);
  }
  else if (velocity.x != 0.0) {
    // Perform de-acceleration whenever we are not accelerating along the x-axis and velocity is non-zero
    velocity.x = clamp(velocity.x + (velocity.x > 0.0 ? -0.05 : 0.05), -1.0, max_velocity.x);
    // Perform precision reset by setting the velocity along the x-axis to zero whenever the velocity is very close
    // to zero but not quite there, avoids problems.
    if (std::abs(velocity.x - 0.01) < 0.1) { velocity.x = 0.0; }
  }

  if (acceleration.y.x) {
    velocity.y = clamp(velocity.y + 0.01 * delta / 16.0, 0.0, max_velocity.y);
  }
  else if (acceleration.y.y) {
    velocity.y = clamp(velocity.y - 0.01 * delta / 16.0, -1.0, 0.0);
  }
  else if (velocity.y != 0.0) {
    velocity.y = clamp(velocity.y + (velocity.y > 0.0 ? -0.05 : 0.05), -1.0, max_velocity.y);
    if (std::abs(velocity.y - 0.01) < 0.1) { velocity.y = 0.0; }
  }

  if (acceleration.z.x) {
    velocity.z = clamp(velocity.z + 0.01 * delta / 16.0, 0.0, max_velocity.z);
  }
  else if (acceleration.z.y) {
    velocity.z = clamp(velocity.z - 0.01 * delta / 16.0, -1.0, 0.0);
  }
  else if (velocity.z != 0.0) {
    velocity.z = clamp(velocity.z + (velocity.z > 0.0 ? -0.05 : 0.05), -1.0, max_velocity.z);
    if (std::abs(velocity.z - 0.01) < 0.1) { velocity.z = 0.0; }
  }

  return position + direction * velocity.x + Vec3<float>::Y() * velocity.y + direction.cross(up).normalize() * velocity.z;
}

glm::mat4 Camera::transform() const {
  glm::vec3 d(direction.x, direction.y, direction.z);
  glm::vec3 u(up.x, up.y, up.z);
  glm::vec3 p(position.x, position.y, position.z);
  return glm::lookAt(p, p + d, u);
}
