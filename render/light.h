#ifndef MEINEKRAFT_LIGHT_H
#define MEINEKRAFT_LIGHT_H

#include "../math/vector.h"

struct Light {
    Color4<float> light_color;
    Vec3<float> position;

    Light(Vec3<float> position): position(position), light_color(Color4<float>::BLUE()) {};
    Light(Vec3<float> position, Color4<float> rgb_color): position(position), light_color(rgb_color) {};

    friend std::ostream &operator<<(std::ostream &os, const Light &light) {
        return os << light.position;
    }
};

#endif //MEINEKRAFT_LIGHT_H
