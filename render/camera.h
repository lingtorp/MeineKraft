#ifndef MEINEKRAFT_CAMERA_H
#define MEINEKRAFT_CAMERA_H

#include "../math/vector.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/rotate_vector.hpp>

struct Camera {
    Camera(Vec3<float> position, Vec3<float> direction, Vec3<float> up);

    /// Where x-axis is direction (forward from the camera) and the other axis are relative to it
    Vec3<float> direction;

    /// Position of the camera
    Vec3<float> position;

    /// Local up of the camera
    Vec3<float> up;

    /// Mouse diff vector from last frame
    Vec2<float> diff_vector;
   
    void update();

    /// Calculates the a new direction vector based on the current rotation
    glm::mat4 transform() const;
};

#endif // MEINEKRAFT_CAMERA_H
