#ifndef MEINEKRAFT_RAY_H
#define MEINEKRAFT_RAY_H

#include "../math/vector.h"

class Ray {
public:
    Vec3<> origin;
    Vec3<> direction;

    Ray() : origin(Vec3<>::ZERO()), direction(Vec3<>::ZERO()) {}
    Ray(Vec3<> position, Vec3<> direction) :
            origin(position), direction(direction) {}

    inline bool hits_sphere(Vec3<> sphere_center, double sphere_radius) {
        // dot(p(t) - C, p(t) - C) - R^2 = 0
        // p(t) = A + t*B // ray
        // C = (x, y, z) // position of the sphere
        auto oc = origin - sphere_center;
        auto a  = direction.dot(direction);
        auto b  = 2.0 * direction.dot(oc);
        auto c  = oc.dot(oc) - sphere_radius * sphere_radius;
        auto discriminant = b*b - 4*a*c;
        return (discriminant > 0);
    }
};

#endif //MEINEKRAFT_RAY_H