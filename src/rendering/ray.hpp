#pragma once
#ifndef MEINEKRAFT_RAY_HPP
#define MEINEKRAFT_RAY_HPP

#include "../math/vector.hpp"

class Ray {
public:
    Vec3f origin;
    Vec3f direction;

    Ray() : origin(Vec3f::ZERO(), direction(Vec3f::ZERO()) {}
    Ray(Vec3f position, Vec3f direction): origin(position), direction(direction) {}

    inline bool hits_sphere(Vec3f sphere_center, double sphere_radius) {
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

#endif // MEINEKRAFT_RAY_HPP