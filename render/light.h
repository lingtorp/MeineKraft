#ifndef MEINEKRAFT_LIGHT_H
#define MEINEKRAFT_LIGHT_H

#include "../math/vector.h"

// FIXME: Should be independent on shader Light struct layout (Vec4 --> Vec3)
struct Light {
    Color4<float> light_color;
    // Intensities over RGB
    Vec4<float> ambient_intensity;
    Vec4<float> diffuse_intensity;
    Vec4<float> specular_intensity;
    Vec4<float> position;
  
    explicit Light(Vec3<float> position): position(position), light_color{1.0}, ambient_intensity{1.0}, diffuse_intensity{1.0}, specular_intensity{1.0} {};

    friend std::ostream &operator<<(std::ostream &os, const Light &light) {
        return os << light.position;
    }
};

#endif //MEINEKRAFT_LIGHT_H
