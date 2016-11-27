#ifndef MEINEKRAFT_LIGHT_H
#define MEINEKRAFT_LIGHT_H

#include "../math/vector.h"

struct Light {
    Vec3<float> position; // w is ignored ...
//    Color4<float> light_color;

    Light(Vec3<float> position): position(position) {};
    // Light(Vec3<float> position): position(position), light_color(Color4<float>::WHITE()) {};
};

#endif //MEINEKRAFT_LIGHT_H
