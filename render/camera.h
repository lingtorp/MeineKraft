#ifndef MEINEKRAFT_CAMERA_H
#define MEINEKRAFT_CAMERA_H

#include "../math/vector.h"

class Camera {
public:
    Vec3<float> direction, position, up;
    float pitch, yaw;
    double velocity; // meters per milliseconds
    double acceleration; // meters / milliseconds^2
    double max_acceleration;

    Camera(Vec3<float> position, Vec3<float> direction, Vec3<float> world_up):
            position(position), direction(direction), up(world_up),
            pitch(0), yaw(0), velocity(0), acceleration(0.03), max_acceleration(3) {};

    Vec3<float> move_forward(double delta);
    Vec3<float> move_backward(double delta);
    Vec3<float> move_right(double delta) const;
    Vec3<float> move_left(double delta) const;
    Vec3<float> recalculate_direction() const;
};

#endif //MEINEKRAFT_CAMERA_H
