#ifndef MEINEKRAFT_TRANSFORM_H
#define MEINEKRAFT_TRANSFORM_H

#include "../math/vector.h"

/**
 * There is a oppotunity here to create some template meta-programming madness with transform settings and transforms.
 * 1. Create a transform-settings struct to contain all the crazy options a transform can have
 * 2. Pass that into a factory which will generate the source for each of the transforms
 */

struct Transform {
    bool finished;

    const Vec3<float> from_position;
    Vec3<float> current_position;
    const Vec3<float> to_position;

    /// Time in milliseconds
    uint64_t time_elapsed;
    const uint64_t duration;

    Transform(Vec3<float> from, Vec3<float> to, uint64_t duration): from_position(from), to_position(to), current_position(from), time_elapsed(0), duration(duration), finished(false) {};

    void update(uint64_t delta) {
        if (time_elapsed >= duration) { finished = true; return; }
        time_elapsed += delta;
        current_position.x = 30 * sinf(time_elapsed * M_PI_2 / 1000);
        current_position.z = 30 * cosf(time_elapsed * M_PI_2 / 1000);
    }
};

#endif //MEINEKRAFT_TRANSFORM_H
