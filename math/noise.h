#ifndef MEINEKRAFT_NOISE_H
#define MEINEKRAFT_NOISE_H

#include <random>
#include "vector.h"

class Noise {
    int seed = 1;
    std::mt19937 engine;
    std::uniform_real_distribution<> distr;
    std::vector<Vec2<double>> grads; /// Normalized gradients
    std::vector<int> perms;

public:
    Noise(): engine(seed), grads(256), distr(0, 1), perms(256) {
        /// Fill the gradients list with random normalized vectors
        for (int i = 0; i < grads.size(); i++) {
            double x = distr(engine);
            double y = distr(engine);
            grads[i] = Vec2<double>{x, y};
        }

        /// Fill gradient lookup array with random indices to the gradients list
        /// Fill with indices from 0 to perms.size()
        std::iota(perms.begin(), perms.end(), 0);

        /// Randomize the order of the indices
        std::shuffle(perms.begin(), perms.end(), engine);
    }

    double perlin(int x, int y, Vec3<> chunk_pos, int dimension) {
        /// Compress the coordinates inside the chunk; double part + int part = point coordinate
        double a = y % dimension; // Integer offset inside the chunk
        double yf = 1 - std::abs(a / dimension); // Float offset inside the chunk (0, 1)
        double yi = chunk_pos.z / dimension; // Integer bounds from the world
        double Y = yf + yi; // Relative position inside the chunk and the chunk from the world coords perspective

        double b = x % dimension;
        double xf = 1 - std::abs(b / dimension);
        double xi = chunk_pos.x / dimension;
        double X = xf + xi;

        /// Grid points from the chunk in the world
        int X0 = (int) (chunk_pos.x / dimension);
        int X1 = (int) (chunk_pos.x + dimension) / dimension;
        int Y0 = (int) chunk_pos.z / dimension;
        int Y1 = (int) (chunk_pos.z + dimension) / dimension;

        /// Gradients using hashed indices from lookup list
        Vec2<double> x0y0 = grads[perms[(X0 + perms[Y0 % perms.size()]) % perms.size()]];
        Vec2<double> x1y0 = grads[perms[(X1 + perms[Y0 % perms.size()]) % perms.size()]];
        Vec2<double> x0y1 = grads[perms[(X0 + perms[Y1 % perms.size()]) % perms.size()]];
        Vec2<double> x1y1 = grads[perms[(X1 + perms[Y1 % perms.size()]) % perms.size()]];

        /// Vectors from gradients to point in unit squere
        auto v00 = Vec2<double>{X0 - X, Y0 - Y};
        auto v10 = Vec2<double>{X1 - X, Y0 - Y};
        auto v01 = Vec2<double>{X0 - X, Y1 - Y};
        auto v11 = Vec2<double>{X1 - X, Y1 - Y};

        /// Contribution of gradient vectors by dot product between relative vectors and gradients
        double d00 = x0y0.dot(v00);
        double d10 = x1y0.dot(v10);
        double d01 = x0y1.dot(v01);
        double d11 = x1y1.dot(v11);

        /// Interpolate dot product values at sample point using polynomial interpolation 6x^5 - 15x^4 + 10x^3
        auto wx = fade(xf);
        auto wy = fade(yf);

        auto xa = lerp(wx, d00, d10);
        auto xb = lerp(wx, d01, d11);
        auto ya = lerp(wy, xa, xb);

        return ya * 10;
    }

    double generate(int x, int y) {
        return 4 * std::cos(x/4) + std::sin(y/2);
    }

    double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    double lerp(double t, double a, double b) { return a + t * (b - a); }
};

#endif //MEINEKRAFT_NOISE_H
