#include "camera.h"

#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <SDL_log.h>
#else
#include <SDL2/SDL_log.h>
#endif 

Camera::Camera(Vec3<float> position, Vec3<float> direction, Vec3<float> up):
        position(position), direction(direction.normalize()), up(up.normalize()) {};

void Camera::update() {
  const float sensitivity = 1.0f;
  const float dx = diff_vector.x * sensitivity;
  const float dy = diff_vector.y * sensitivity;

  glm::vec3 direction(direction.x, direction.y, direction.z);
  glm::vec3 up(up.x, up.y, up.z);
  glm::vec3 position(position.x, position.y, position.z);

  std::cerr << glm::to_string(direction) << std::endl;

  glm::vec3 rotation = glm::rotate(direction, glm::radians(dx), up);

  std::cerr << dx << std::endl;
  std::cerr << glm::to_string(rotation) << std::endl;

  this->direction = Vec3<float>(rotation.x, rotation.y, rotation.z);
  direction = glm::vec3(direction.x, direction.y, direction.z);

  rotation = glm::rotate(direction, glm::radians(dy), glm::cross(up, direction));
  glm::vec3 tilt = glm::rotate(up, glm::radians(dy), glm::cross(up, direction));

  this->direction = Vec3<float>(rotation.x, rotation.y, rotation.z);
  this->up = Vec3<float>(tilt.x, tilt.y, tilt.z);
}

glm::mat4 Camera::transform() const {
  glm::vec3 direction(direction.x, direction.y, direction.z);
  glm::vec3 up(up.x, up.y, up.z);
  glm::vec3 position(position.x, position.y, position.z);
  return glm::lookAt(direction, position, up);
}