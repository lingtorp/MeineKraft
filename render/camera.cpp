#include "camera.h"

Vec3<> Camera::move_forward(double delta) const {
    Vec3<> movement = direction * (movement_speed * delta);
    return position + movement;
}

Vec3<> Camera::move_backward(double delta) const {
    Vec3<> movement = direction * (movement_speed * delta);
    return position - movement;
}

Vec3<> Camera::move_right(double delta) const {
    Vec3<> movement = direction.cross(up).normalize();
    return position + (movement * (movement_speed * delta));
}

Vec3<> Camera::move_left(double delta) const {
    Vec3<> movement = direction.cross(up).normalize();
    return position - (movement * (movement_speed * delta));
}

Vec3<> Camera::recalculate_direction() const {
    static constexpr double rad = M_PI / 180;
    Vec3<> result;
    result.x = -sin(yaw * rad) * cos(pitch * rad);
    result.y = sin(pitch * rad);
    result.z = -cos(yaw * rad) * cos(pitch * rad);
    return result.normalize();
}