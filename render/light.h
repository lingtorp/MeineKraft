#ifndef MEINEKRAFT_LIGHT_H
#define MEINEKRAFT_LIGHT_H

#include "../math/vector.h"

// FIXME: Should be independent on shader Light struct layout (Vec4 --> Vec3)
struct PointLight {
    Color3<float> color;
    // Intensities over RGB
    Vec3<float> ambient_intensity;
    Vec3<float> diffuse_intensity;
    Vec3<float> specular_intensity;
    Vec3<float> position;
  
    explicit PointLight(Vec3<float> position): position(position), color{1.0}, ambient_intensity{1.0}, diffuse_intensity{1.0}, specular_intensity{1.0} {};

    friend std::ostream &operator<<(std::ostream &os, const PointLight &light) {
        return os << light.position;
    }
};

#endif //MEINEKRAFT_LIGHT_H
