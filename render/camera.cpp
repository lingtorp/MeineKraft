#include "camera.h"

Vec3<float> Camera::move_forward(double delta) {
    velocity = std::min(velocity + acceleration * delta, max_velocity);
    return position + direction * velocity;
}

Vec3<float> Camera::move_backward(double delta) {
    velocity = std::min(velocity + acceleration * delta, -max_velocity);
    return position + direction * velocity;
}

Vec3<float> Camera::move_right(double delta) const {
    Vec3<float> movement = direction.cross(up).normalize();
    return position + (movement * (0.5 * delta));
}

Vec3<float> Camera::move_left(double delta) const {
    Vec3<float> movement = direction.cross(up).normalize();
    return position - (movement * (0.5 * delta));
}

Vec3<float> Camera::recalculate_direction() const {
    static constexpr float rad = M_PI / 180;
    Vec3<float> result;
    result.x = -sinf(yaw * rad) * cosf(pitch * rad);
    result.y = sinf(pitch * rad);
    result.z = -cosf(yaw * rad) * cosf(pitch * rad);
    return result.normalize();
}

void Camera::update(double delta) {
    acceleration = std::max(std::abs(acceleration - 0.1 * delta), 0.0);
}
