#ifndef MEINEKRAFT_LIGHT_H
#define MEINEKRAFT_LIGHT_H

#include "../math/vector.h"

struct Light {
    Color4<float> light_color;
    Vec4<float> light_itensity; // (ambient, diffuse, specular, padding) itensity
    Vec3<float> position;

    Light(Vec3<float> position): position(position), light_color(Color4<float>::BLUE()), light_itensity{0.5, 0.5, 0.5} {};
    Light(Vec3<float> position, Color4<float> rgb_color): position(position), light_color(rgb_color), light_itensity{0.5, 0.5, 0.5} {};

    friend std::ostream &operator<<(std::ostream &os, const Light &light) {
        return os << light.position;
    }
};

#endif //MEINEKRAFT_LIGHT_H
