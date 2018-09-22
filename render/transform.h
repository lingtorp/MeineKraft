#ifndef MEINEKRAFT_TRANSFORM_H
#define MEINEKRAFT_TRANSFORM_H

#include "../math/vector.h"

struct Transform {
    Vec3<float> current_position;

    /// Time in milliseconds
    int64_t time_elapsed;

    Transform(): current_position{}, time_elapsed(0) {};

    void update(int64_t delta) {
        time_elapsed += delta;
        current_position.x = 1.0f * sinf(time_elapsed * M_PI / 1000.0f);
        current_position.y = 1.0f * cosf(time_elapsed * M_PI / 1000.0f);
    }
};

#endif // MEINEKRAFT_TRANSFORM_H
