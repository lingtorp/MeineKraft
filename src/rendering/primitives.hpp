#pragma once
#ifndef MEINEKRAFT_PRIMITIVES_HPP
#define MEINEKRAFT_PRIMITIVES_HPP

#include <vector>

#include "../math/vector.hpp"

/// Mathematical constants
constexpr double PI = 3.1415926535897932384626433832795;

/// Linear interpolation of a, b given t
static float lerp(const float a, const float b, const float t) {
  return a + t * (b - a);
}

/// Colors
template<typename T>
class Color4 {
public:
  T r, g, b, a = 0;
  constexpr explicit Color4(T val): r(val), g(val), b(val), a(val) {};
  constexpr Color4() = default;
  constexpr Color4(T r, T g, T b, T a): r(r), g(g), b(b), a(a) {};

  bool operator==(const Color4<T> &rhs) const {
      return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
  }
};

template<typename T>
class Color3 {
public:
  T r, g, b;
  constexpr explicit Color3(T val): r(val), g(val), b(val) {};
  constexpr Color3() = default;
  constexpr Color3(T r, T g, T b): r(r), g(g), b(b) {};
};

struct Vertex {
  Vec3f position  = {};
  Vec2f tex_coord = {};
  Vec3f normal    = {};
  Vertex() = default;
  explicit Vertex(Vec3f position): position(position), tex_coord{}, normal{} {};
  Vertex(Vec3f position, Vec2f tex_coord): position(position), tex_coord(tex_coord), normal{} {};

  bool operator==(const Vertex& rhs) const {
      return position == rhs.position && tex_coord == rhs.tex_coord && normal == rhs.normal;
  }
};

/// The name says it all really
inline void hash_combine(size_t& seed, const size_t hash) {
  seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

/// Template specialization for hashing of a Vertex
namespace std {
  template<>
  struct hash<Vertex> {
      size_t operator() (const Vertex& vertex) const {
          auto hasher = hash<float>{};
          auto hashed_x = hasher(vertex.position.x);
          auto hashed_y = hasher(vertex.position.y);
          auto hashed_z = hasher(vertex.position.z);
          auto hashed_texcoord_x = hasher(vertex.tex_coord.x);
          auto hashed_texcoord_y = hasher(vertex.tex_coord.y);
          auto hashed_normal_x = hasher(vertex.normal.x);
          auto hashed_normal_y = hasher(vertex.normal.y);
          auto hashed_normal_z = hasher(vertex.normal.z);

          size_t seed = 0;
          hash_combine(seed, hashed_x);
          hash_combine(seed, hashed_y);
          hash_combine(seed, hashed_z);
          hash_combine(seed, hashed_texcoord_x);
          hash_combine(seed, hashed_texcoord_y);
          hash_combine(seed, hashed_normal_x);
          hash_combine(seed, hashed_normal_y);
          hash_combine(seed, hashed_normal_z);
          return seed;
      }
  };
}

/// Template specialization for hashing of a Vec3
namespace std {
  template<typename T>
  struct hash<Vec3<T>> {
      size_t operator() (const Vec3<T>& vec) const {
          auto hasher = hash<float>{};
          auto hashed_x = hasher(vec.x);
          auto hashed_y = hasher(vec.y);
          auto hashed_z = hasher(vec.z);

          size_t seed = 0;
          hash_combine(seed, hashed_x);
          hash_combine(seed, hashed_y);
          hash_combine(seed, hashed_z);
          return seed;
      }
  };
}

/// Represents primitive types of meshes supported
/// MeshPrimitives are their own mesh IDs
enum class MeshPrimitive: uint32_t {
  Cube, CubeCounterClockWinding, Sphere, Quad
};

struct Mesh {
  std::vector<Vertex> vertices{};
  std::vector<uint32_t> indices{};

  Mesh() = default;
  Mesh(const Mesh& mesh): vertices(mesh.vertices), indices(mesh.indices) {};
  Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices): vertices(vertices), indices(indices) {};

  /// Byte size of vertices to upload to OpenGL
  inline size_t byte_size_of_vertices() const {
      return sizeof(Vertex) * vertices.size();
  }

  /// Byte size of indices to upload to OpenGL
  inline size_t byte_size_of_indices() const {
      return sizeof(uint32_t) * indices.size();
  }
};

/// Unit cube
struct Cube: public Mesh {
  Cube(const bool counter_clock_winding = false): Mesh() {
      const auto a = Vec3f(-0.5f, -0.5f, 0.5f);
      const auto b = Vec3f(0.5f, -0.5f, 0.5f);
      const auto c = Vec3f(0.5f, 0.5f, 0.5f);
      const auto d = Vec3f(-0.5f, 0.5f, 0.5f);
      const auto tex_a = Vec2f(0.0f, 0.0f);
      const auto tex_b = Vec2f(1.0f, 0.0f);
      const auto tex_c = Vec2f(1.0f, 1.0f);
      const auto tex_d = Vec2f(0.0f, 1.0f);
      vertices.emplace_back(Vertex(a, tex_a));
      vertices.emplace_back(Vertex(b, tex_b));
      vertices.emplace_back(Vertex(c, tex_c));
      vertices.emplace_back(Vertex(d, tex_d));

      const auto e = Vec3f(-0.5f, -0.5f, -0.5f);
      const auto f = Vec3f(0.5f, -0.5f, -0.5f);
      const auto g = Vec3f(0.5f, 0.5f, -0.5f);
      const auto h = Vec3f(-0.5f, 0.5f, -0.5f);
      const auto tex_e = Vec2f(1.0f, 0.0f);
      const auto tex_f = Vec2f(0.0f, 0.0f);
      const auto tex_g = Vec2f(0.0f, 1.0f);
      const auto tex_h = Vec2f(1.0f, 1.0f);
      vertices.emplace_back(Vertex(e, tex_e));
      vertices.emplace_back(Vertex(f, tex_f));
      vertices.emplace_back(Vertex(g, tex_g));
      vertices.emplace_back(Vertex(h, tex_h));

      // Used in the skybox (when you want to see the inside of a cube)
      if (counter_clock_winding) {
        indices = { // front
            2, 1, 0, 0, 3, 2,
            // right
            6, 5, 1, 1, 2, 6,
            // back
            5, 6, 7, 7, 4, 5,
            // left
            3, 0, 4, 4, 7, 3,
            // bot
            1, 5, 4, 4, 0, 1,
            // top
            6, 2, 3, 3, 7, 6 };
      } else {
        indices = { // front
            0, 1, 2, 2, 3, 0,
            // right
            1, 5, 6, 6, 2, 1,
            // back
            7, 6, 5, 5, 4, 7,
            // left
            4, 0, 3, 3, 7, 4,
            // bot
            4, 5, 1, 1, 0, 4,
            // top
            3, 2, 6, 6, 7, 3 };
      }
  }
};

/// Sphere mesh
struct Sphere: public Mesh {
  explicit Sphere(const float radius = 1.0f): Mesh() {
    const uint32_t X_SEGMENTS = 64;
    const uint32_t Y_SEGMENTS = X_SEGMENTS;

    for (uint32_t j = 0; j <= Y_SEGMENTS; ++j) {
      for (uint32_t i = 0; i <= X_SEGMENTS; ++i) {
        const float x_segment = (float)i / (float)X_SEGMENTS;
        const float y_segment = (float)j / (float)Y_SEGMENTS;
        const float x = mk_cos(x_segment * 2.0f * PI) * mk_sin(y_segment * PI);
        const float y = mk_cos(y_segment * PI);
        const float z = mk_sin(x_segment * 2.0f * PI) * mk_sin(y_segment * PI);
        Vertex vertex;
        vertex.position = Vec3f{x, y, z} * radius;
        vertex.normal = Vec3f{x, y, z} * radius;
        vertices.emplace_back(vertex);

        if (j <= Y_SEGMENTS) {
          const uint32_t curRow  = j * X_SEGMENTS;
          const uint32_t nextRow = (j + 1) * X_SEGMENTS;
          const uint32_t nextS   = (i + 1) % X_SEGMENTS;

          indices.push_back(nextRow + nextS);
          indices.push_back(nextRow + i);
          indices.push_back(curRow + i);

          indices.push_back(curRow + nextS);
          indices.push_back(nextRow + nextS);
          indices.push_back(curRow + i);
        }
      }
    }
  }
};

// TODO: Use it, remove Primitive::quad
/// Fullscreen quad in NDC
struct Quad: public Mesh {
  Quad(): Mesh() {
    Vertex a;
    a.position = {-1.0f, -1.0f, 1.0f};
    a.tex_coord = {0.0f, 1.0f};
    Vertex b;
    b.position = {-1.0f, -1.0f, 0.0f};
    b.tex_coord = {0.0f, 0.0f};
    Vertex c;
    c.position = {1.0f,  1.0f, 0.0f};
    c.tex_coord = {1.0f, 1.0f};
    Vertex d;
    d.position = {1.0f, -1.0f, 0.0f};
    d.tex_coord = {1.0f, 0.0f};
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);
    vertices.push_back(d);
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(3);
  }
};

namespace Primitive {
  /// Fullscreen quad in NDC
  static float quad[] = {
    // positions        // texture Coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
  };
}

/// Mathematical plane: a*x + b*y + c*z = d
template<typename T>
struct Plane {
  T a, b, c, d;

  Plane(): a(0), b(0), c(0), d(0) {};
  Plane(T a, T b, T c, T d): a(a), b(b), c(c), d(d) {};

  /// Normal of the plane
  /// @return Normal vector of the plane
  inline Vec3<T> normal() const {
      return Vec3<T>(a, b, c);
  }

  /// Distance to point from the plane
  /// distance < 0, then point lies in the negative halfspace
  /// distance = 0, then point lies in the plane
  /// distance > 0, then point lies in the positive halfspace
  inline double distance_to_point(const Vec3<T>& point) const {
      return a*point.x + b*point.y + c*point.z + d;
  }
};

/// Opaque ID type used to reference resources throughout the engine
typedef uint64_t ID;

enum class ShadingModel: uint32_t {
  Unlit = 1,                        // Unlit, using its surface color 
  PhysicallyBased = 2,              // PBR using textures (default)
  PhysicallyBasedScalars = 3        // PBR using scalars instead of textures
};

/// Represents the state of the Render, used for ImGUI debug panes
struct RenderState {
  uint64_t frame           = 0;
  uint64_t entities        = 0;
  uint64_t graphic_batches = 0;
  uint64_t draw_calls      = 0;
  bool shadowmapping = false;
  RenderState() = default;
  RenderState(const RenderState& old): frame(old.frame), shadowmapping(old.shadowmapping) {}
};

struct Resolution {
  int width, height;
};

static auto HD      = Resolution{1280, 720};
static auto FULL_HD = Resolution{1920, 1080};

#endif // MEINEKRAFT_PRIMITIVES_HPP
