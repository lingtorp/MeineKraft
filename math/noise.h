#ifndef MEINEKRAFT_NOISE_H
#define MEINEKRAFT_NOISE_H

#include <random>

namespace noise {
    double perlin(double x, double y) {
        static std::default_random_engine engine;
        static std::uniform_real_distribution<> distr(0, 1);
        return distr(engine);
    }
}

#endif //MEINEKRAFT_NOISE_H
