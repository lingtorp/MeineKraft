#ifndef MEINEKRAFT_CAMERA_H
#define MEINEKRAFT_CAMERA_H

#include "../math/vector.h"

// TODO: Improve the camera
class Camera {
public:
    Vec3<float> direction, position, up;
    float pitch, yaw;

    double velocity; // meters per milliseconds
    double max_velocity;

    double acceleration; // meters / milliseconds^2

    Camera(Vec3<float> position, Vec3<float> direction, Vec3<float> world_up);

    void update(uint32_t delta);
    Vec3<float> move_forward(double delta);
    Vec3<float> move_backward(double delta);
    Vec3<float> move_right(double delta) const;
    Vec3<float> move_left(double delta) const;
    Vec3<float> recalculate_direction() const;
};

#endif //MEINEKRAFT_CAMERA_H
