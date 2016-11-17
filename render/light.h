#ifndef MEINEKRAFT_LIGHT_H
#define MEINEKRAFT_LIGHT_H

#include "../math/vector.h"

struct Light {
    Vec3<float> position;

    Light(Vec3<float> position): position(position) {};
};

#endif //MEINEKRAFT_LIGHT_H
