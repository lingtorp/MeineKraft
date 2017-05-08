#ifndef MEINEKRAFT_CAMERA_H
#define MEINEKRAFT_CAMERA_H

#include "../math/vector.h"

class Camera {
public:
    /// Where x-axis is direction (forward from the camera) and the other axis are relative to it
    Vec3<float> direction;

    /// Position of the camera
    Vec3<float> position;

    /// Global positive up vector (y-axis)
    Vec3<float> up;

    /// Pitch and yaw of the camera
    float pitch, yaw;

    /// Velocity in the three axis
    Vec3<double> velocity;

    /// Max velocity of the movement along the three axis
    const Vec3<double> max_velocity;

    /**
     * Acceleration indicates the direction of the axis that are moving
     * Acceleration: {x-axis{postive-direction, negative-direction}, ... }
     */
    Vec3<Vec2<bool>> acceleration;

    Camera(Vec3<float> position, Vec3<float> direction, Vec3<float> world_up);

    /// Update performs the acceleration calculations and returns the new position
    Vec3<float> update(uint32_t delta);

    // Start and stop the accelerations in the directions
    void move_forward(bool move);
    void move_backward(bool move);
    void move_right(bool move);
    void move_left(bool move);
    void move_down(bool move);
    void move_up(bool move);

    /// Calculates the a new direction vector based on the current pitch and yaw
    Vec3<float> recalculate_direction() const;
};

#endif //MEINEKRAFT_CAMERA_H
