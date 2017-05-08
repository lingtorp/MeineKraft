#include <SDL2/SDL_log.h>
#include <algorithm>
#include "camera.h"

/// Clamps a number to between lo and hi, in other words: [lo, hi]
static double clamp(double x, double lo, double hi) {
    return std::min(std::max(x, lo), hi);
}

Camera::Camera(Vec3<float> position, Vec3<float> direction, Vec3<float> world_up):
        position(position), direction(direction), up(world_up),
        pitch(0), yaw(0), velocity{0.0, 0.0, 0.0}, max_velocity{0.5, 0.5, 0.5}, acceleration{{false, false}, {false, false}, {false, false}} {};

void Camera::move_forward(bool move) {
    acceleration.x.x = move;
}

void Camera::move_backward(bool move) {
    acceleration.x.y = move;
}

void Camera::move_right(bool move) {
    acceleration.z.x = move;
}

void Camera::move_left(bool move){
    acceleration.z.y = move;
}

void Camera::move_down(bool move) {
    acceleration.y.x = move;
}

void Camera::move_up(bool move) {
    acceleration.y.y = move;
}

Vec3<float> Camera::recalculate_direction() const {
    static constexpr float rad = M_PI / 180.0f;
    Vec3<float> result;
    result.x = -sinf(yaw * rad) * cosf(pitch * rad);
    result.y = sinf(pitch * rad);
    result.z = -cosf(yaw * rad) * cosf(pitch * rad);
    return result.normalize();
}

Vec3<float> Camera::update(uint32_t delta) {
    /// Perform acceleration and de-acceleration calculations for each direction on each axis.
    if (acceleration.x.x) {
        // If accelerating along positive x-axis, increase the velocity along the positive axis
        velocity.x = clamp(velocity.x + 0.01 * delta/16.0,  0.0, max_velocity.x);
    } else if (acceleration.x.y) {
        // If accelerating along negative x-axis, increase the velocity along the negative axis
        velocity.x = clamp(velocity.x - 0.01 * delta/16.0, -1.0, 0.0);
    } else if (velocity.x != 0.0) {
        // Perform de-acceleration whenever we are not accelerating along the x-axis and velocity is non-zero
        velocity.x = clamp(velocity.x + (velocity.x > 0.0 ? -0.05 : 0.05), -1.0, max_velocity.x);
        // Perform precision reset by setting the velocity along the x-axis to zero whenever the velocity is very close
        // to zero but not quite there, avoids problems.
        if (std::abs(velocity.x - 0.01) < 0.1) { velocity.x = 0.0; }
    }

    if (acceleration.y.x) {
        velocity.y = clamp(velocity.y + 0.01 * delta/16.0,  0.0, max_velocity.y);
    } else if (acceleration.y.y) {
        velocity.y = clamp(velocity.y - 0.01 * delta/16.0, -1.0, 0.0);
    } else if (velocity.y != 0.0) {
        velocity.y = clamp(velocity.y + (velocity.y > 0.0 ? -0.05 : 0.05), -1.0, max_velocity.y);
        if (std::abs(velocity.y - 0.01) < 0.1) { velocity.y = 0.0; }
    }

    if (acceleration.z.x) {
        velocity.z = clamp(velocity.z + 0.01 * delta/16.0,  0.0, max_velocity.z);
    } else if (acceleration.z.y) {
        velocity.z = clamp(velocity.z - 0.01 * delta/16.0, -1.0, 0.0);
    } else if (velocity.z != 0.0) {
        velocity.z = clamp(velocity.z + (velocity.z > 0.0 ? -0.05 : 0.05), -1.0, max_velocity.z);
        if (std::abs(velocity.z - 0.01) < 0.1) { velocity.z = 0.0; }
    }

    return position + direction * velocity.x + Vec3<float>::Y() * velocity.y + direction.cross(up).normalize() * velocity.z;
}