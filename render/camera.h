#ifndef MEINEKRAFT_CAMERA_H
#define MEINEKRAFT_CAMERA_H

#include "../math/vector.h"

class Camera {
public:
    Vec3 direction, position, up;
    GLfloat pitch, yaw;
    GLfloat movement_speed; // meters per milliseconds

    Camera(Vec3 position, Vec3 direction, Vec3 world_up):
            position(position), direction(direction), up(world_up),
            pitch(0), yaw(0), movement_speed(0.01) {};

    Vec3 move_forward(double delta) const;
    Vec3 move_backward(double delta) const;
    Vec3 move_right(double delta) const;
    Vec3 move_left(double delta) const;
    Vec3 recalculate_direction() const;
};

#endif //MEINEKRAFT_CAMERA_H
