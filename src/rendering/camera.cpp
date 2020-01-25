#include <algorithm>

#include "../math/quaternion.hpp"
#include "../rendering/primitives.hpp"
#include "../util/logging.hpp"

#include "camera.hpp"

Camera::Camera(nlohmann::json json) {
  if (json == nullptr) {
    Log::error("Tried to create a Camera from json with nullptr");
    return;
  }

  if (json.empty()) {
    Log::error("Tried to create a Camera from empty json");
    return;
  }

  const auto scene = json["scene"];
  if (scene.empty()) {
    Log::error("No scene object in json");
    return;
  }

  const auto cam = scene["camera"];
  if (cam.empty()) {
    Log::error("No Camera json object in Scene json");
    return;
  }

  const auto pos = cam["position"];
  if (!pos.empty() && pos.is_array()) {
    if (pos.size() != 3) {
      Log::warn("Invalid array size in Camera.position");
    } else {
      this->position.x = pos[0];
      this->position.y = pos[1];
      this->position.z = pos[2];
    }
  } else {
    Log::warn("No position array in Camera json");
  }

  const auto dir = cam["direction"];
  if (!dir.empty() && dir.is_array()) {
    if (dir.size() != 3) {
      Log::warn("Invalid array size in Camera.direction");
    } else {
      this->direction.x = dir[0];
      this->direction.y = dir[1];
      this->direction.z = dir[2];
    }
  } else {
    Log::warn("No direction array in Camera json");
  }

  // TODO: Implement Camera FoV
  // const auto fov = cam["fov"];
  // if (!fov.empty() && fov.is_float()) {
  //   this->FoV = fov.get<float>();
  // }
}

Camera::Camera(const Vec3f position, Vec3f direction): direction(direction), position(position) {}

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

Vec3f Camera::recalculate_direction() const {
  const bool euler_angles = true;
  if (euler_angles) {
    static constexpr float rad = PI / 180.0f;
    Vec3f result;
    result.x = -sinf(yaw * rad) * cosf(pitch * rad);
    result.y = sinf(pitch * rad);
    result.z = -cosf(yaw * rad) * cosf(pitch * rad);
    return result.normalize();
  } else {
    const float sensitivity = 0.2f;
    const float dx = glm::radians(yaw * sensitivity);
    const float dy = glm::radians(pitch * sensitivity);

    quat rotation(direction.cross(up));
    Vec3f new_direction = rotation.rotate(direction, dy).normalize();
    Vec3f new_up = rotation.rotate(up, dy).normalize();

    quat yrotation(new_up);
    return yrotation.rotate(new_direction, dx).normalize();
  }
}

Vec3f Camera::update(const uint32_t delta) {
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

  return position + direction * velocity.x + Vec3f::Y() * velocity.y + direction.cross(up).normalize() * velocity.z;
}

glm::mat4 Camera::transform() const {
  glm::vec3 d(direction.x, direction.y, direction.z);
  glm::vec3 u(up.x, up.y, up.z);
  glm::vec3 p(position.x, position.y, position.z);
  return glm::lookAt(p, p + d, u);
}
