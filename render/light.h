#ifndef MEINEKRAFT_LIGHT_H
#define MEINEKRAFT_LIGHT_H

#include "../math/vector.h"

static const Mesh sphere = Sphere{};

struct PointLight {
  float radius = 1.0;
  
  // Attenuation values (http://www.ogre3d.org/tikiwiki/tiki-index.php?page=-Point+Light+Attenuation)
  float constant = 1.0;
  float linear = 0.7;
  float quadratic = 1.8;
  
  Color3<float> color;
  // Intensities over RGB
  Vec3<float> ambient_intensity;
  Vec3<float> diffuse_intensity;
  Vec3<float> specular_intensity;
  Vec3<float> position;

  explicit PointLight(Vec3<float> position): position(position), color{1.0}, ambient_intensity{1.0},
                                             diffuse_intensity{1.0}, specular_intensity{1.0} {};

  friend std::ostream &operator<<(std::ostream &os, const PointLight &light) {
      return os << light.position;
  }
};

#endif // MEINEKRAFT_LIGHT_H
