#include "camera.h"

Vec3 Camera::move_forward(double delta) const {
    Vec3 movement =
            vec_scalar_multiplication(direction, movement_speed * delta);
    return vec_addition(position, movement);
}

Vec3 Camera::move_backward(double delta) const {
    Vec3 movement =
            vec_scalar_multiplication(direction, movement_speed * delta);
    return vec_subtraction(position, movement);
}

Vec3 Camera::move_right(double delta) const {
    Vec3 movement = normalize(cross(direction, up));
    return vec_addition(position,
                        vec_scalar_multiplication(movement, movement_speed * delta));
}

Vec3 Camera::move_left(double delta) const {
    Vec3 movement = normalize(cross(direction, up));
    return vec_subtraction(
            position, vec_scalar_multiplication(movement, movement_speed * delta));
}

Vec3 Camera::recalculate_direction() const {
    float rad = M_PI / 180;
    Vec3 result;
    result.x = -sin(yaw * rad) * cos(pitch * rad);
    result.y = sin(pitch * rad);
    result.z = -cos(yaw * rad) * cos(pitch * rad);
    return normalize(result);
}