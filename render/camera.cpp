#include "camera.h"

Vec3<float> Camera::move_forward(double delta) {
    velocity = std::min(velocity + acceleration * delta, max_acceleration);
    return position + direction * velocity;
}

Vec3<float> Camera::move_backward(double delta) {
    velocity = std::min(velocity + acceleration * delta, -max_acceleration);
    return position + direction * velocity;
}

Vec3<float> Camera::move_right(double delta) const {
    Vec3<float> movement = direction.cross(up).normalize();
    return position + (movement * (acceleration * delta));
}

Vec3<float> Camera::move_left(double delta) const {
    Vec3<float> movement = direction.cross(up).normalize();
    return position - (movement * (acceleration * delta));
}

Vec3<float> Camera::recalculate_direction() const {
    static constexpr double rad = M_PI / 180;
    Vec3<float> result;
    result.x = -sin(yaw * rad) * cos(pitch * rad);
    result.y = sin(pitch * rad);
    result.z = -cos(yaw * rad) * cos(pitch * rad);
    return result.normalize();
}