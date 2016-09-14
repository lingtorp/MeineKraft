#ifndef MEINEKRAFT_CAMERA_H
#define MEINEKRAFT_CAMERA_H

#include "../math/vector.h"

class Camera {
public:
    Vec3<GLfloat> direction, position, up;
    GLfloat pitch, yaw;
    double velocity; // meters per milliseconds
    double acceleration; // meters / milliseconds^2
    double max_acceleration;

    Camera(Vec3<> position, Vec3<> direction, Vec3<> world_up):
            position(position), direction(direction), up(world_up),
            pitch(0), yaw(0), velocity(0), acceleration(0.02), max_acceleration(2) {};

    Vec3<> move_forward(double delta);
    Vec3<> move_backward(double delta);
    Vec3<> move_right(double delta) const;
    Vec3<> move_left(double delta) const;
    Vec3<> recalculate_direction() const;
};

#endif //MEINEKRAFT_CAMERA_H
