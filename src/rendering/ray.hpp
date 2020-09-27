#pragma once
#ifndef MEINEKRAFT_RAY_HPP
#define MEINEKRAFT_RAY_HPP

#include "../math/vector.hpp"

struct Ray {
    Vec3f origin;
    Vec3f direction;

    explicit Ray(): origin(Vec3f::ZERO(), direction(Vec3f::ZERO()) {}
    Ray(Vec3f position, Vec3f direction): origin(position), direction(direction) {}

    inline bool hits_sphere(Vec3f sphere_center, double sphere_radius) const {
        // dot(p(t) - C, p(t) - C) - R^2 = 0
        // p(t) = A + t*B // ray
        // C = (x, y, z) // position of the sphere
        const Vec3f oc = origin - sphere_center;
        const float a  = direction.dot(direction);
        const float b  = 2.0 * direction.dot(oc);
        const float c  = oc.dot(oc) - sphere_radius * sphere_radius;
        const float discriminant = b*b - 4*a*c;
        return (discriminant > 0);
    }
};

#endif // MEINEKRAFT_RAY_HPP
