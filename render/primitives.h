#ifndef MEINEKRAFT_PRIMITIVES_H
#define MEINEKRAFT_PRIMITIVES_H

#include <cstdlib>
#include <vector>
#include "../math/vector.h"
#include "texture.h"

/// Mathematical constants
constexpr double PI = 3.1415926535897932384626433832795;

/// Linear interpolation of a, b given t
static float lerp(float a, float b, float t) {
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

// TODO: Template metaprogram these?
template<typename T>
class Color3 {
public:
  T r, g, b;
  constexpr explicit Color3(T val): r(val), g(val), b(val) {};
  constexpr Color3() = default;
  constexpr Color3(T r, T g, T b): r(r), g(g), b(b) {};
};

template<typename T>
struct Vertex {
  Vec3<T>   position = {};
  Color4<T> color    = {};
  Vec2<T>   texCoord = {};
  Vec3<T>   normal   = {};
  Vertex() = default;
  explicit Vertex(Vec3<T> position): position(position), texCoord{}, color{}, normal{} {};
  Vertex(Vec3<T> position, Vec2<T> texCoord): position(position), color{}, texCoord(texCoord), normal{} {};

  bool operator==(const Vertex<T> &rhs) const {
      return position == rhs.position && color == rhs.color && texCoord == rhs.texCoord && normal == rhs.normal;
  }
};

/// Template specialization for hashing of a Vertex
namespace std {
  template<>
  struct hash<Vertex<float>> {
      void hash_combine(size_t &seed, const size_t hash) const {
          seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }

      size_t operator() (const Vertex<float> &vertex) const {
          auto hasher = hash<float>{};
          auto hashed_x = hasher(vertex.position.x);
          auto hashed_y = hasher(vertex.position.y);
          auto hashed_z = hasher(vertex.position.z);
          auto hashed_texcoord_x = hasher(vertex.texCoord.x);
          auto hashed_texcoord_y = hasher(vertex.texCoord.y);
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
      void hash_combine(size_t &seed, const size_t &hash) const {
          seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }

      size_t operator() (const Vec3<T> &vec) const {
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
  Cube, Sphere, Quad
};

struct Mesh {
  std::vector<Vertex<float>> vertices{};
  std::vector<uint32_t> indices{};

  Mesh() = default;
  Mesh(const Mesh& mesh): vertices(mesh.vertices), indices(mesh.indices) {};
  Mesh(std::vector<Vertex<float>> vertices, std::vector<uint32_t> indices): vertices(vertices), indices(indices) {};

  /// Return a vector of all the vertices data laid out as the Vertex struct
  std::vector<float> to_floats() const {
      std::vector<float> floats;
      for (const auto vertex : vertices) {
          floats.push_back(vertex.position.x);
          floats.push_back(vertex.position.y);
          floats.push_back(vertex.position.z);
          floats.push_back(vertex.color.r);
          floats.push_back(vertex.color.g);
          floats.push_back(vertex.color.b);
          floats.push_back(vertex.color.a);
          floats.push_back(vertex.texCoord.x);
          floats.push_back(vertex.texCoord.y);
          floats.push_back(vertex.normal.x);
          floats.push_back(vertex.normal.y);
          floats.push_back(vertex.normal.z);
      }
      return floats;
  }

  /// Byte size of vertices to upload to OpenGL
  size_t byte_size_of_vertices() const {
      return sizeof(Vertex<float>) * vertices.size();
  }

  /// Byte size of indices to upload to OpenGL
  size_t byte_size_of_indices() const {
      return sizeof(uint32_t) * indices.size();
  }
};

/// Unit cube
struct Cube: public Mesh {
  Cube(): Mesh() {
      auto a = Vec3<float>(-0.5f, -0.5f, 0.5f);
      auto b = Vec3<float>(0.5f, -0.5f, 0.5f);
      auto c = Vec3<float>(0.5f, 0.5f, 0.5f);
      auto d = Vec3<float>(-0.5f, 0.5f, 0.5f);
      auto tex_a = Vec2<float>(0.0f, 0.0f);
      auto tex_b = Vec2<float>(1.0f, 0.0f);
      auto tex_c = Vec2<float>(1.0f, 1.0f);
      auto tex_d = Vec2<float>(0.0f, 1.0f);
      vertices.emplace_back(Vertex<float>(a, tex_a));
      vertices.emplace_back(Vertex<float>(b, tex_b));
      vertices.emplace_back(Vertex<float>(c, tex_c));
      vertices.emplace_back(Vertex<float>(d, tex_d));

      auto e = Vec3<float>(-0.5f, -0.5f, -0.5f);
      auto f = Vec3<float>(0.5f, -0.5f, -0.5f);
      auto g = Vec3<float>(0.5f, 0.5f, -0.5f);
      auto h = Vec3<float>(-0.5f, 0.5f, -0.5f);
      auto tex_e = Vec2<float>(1.0f, 0.0f);
      auto tex_f = Vec2<float>(0.0f, 0.0f);
      auto tex_g = Vec2<float>(0.0f, 1.0f);
      auto tex_h = Vec2<float>(1.0f, 1.0f);
      vertices.emplace_back(Vertex<float>(e, tex_e));
      vertices.emplace_back(Vertex<float>(f, tex_f));
      vertices.emplace_back(Vertex<float>(g, tex_g));
      vertices.emplace_back(Vertex<float>(h, tex_h));

      indices =  { // front
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
                  3, 2, 6, 6, 7, 3};
  }
};

/// Sphere mesh
struct Sphere: public Mesh {
  explicit Sphere(const float radius = 1.0f): Mesh() {
    const uint32_t X_SEGMENTS = 64;
    const uint32_t Y_SEGMENTS = X_SEGMENTS;

    for (size_t j = 0; j <= Y_SEGMENTS; ++j) {
      for (size_t i = 0; i <= X_SEGMENTS; ++i) {
        float x_segment = (float)i / (float)X_SEGMENTS;
        float y_segment = (float)j / (float)Y_SEGMENTS;
        float x = std::cos(x_segment * 2.0f * PI) * std::sin(y_segment * PI);
        float y = std::cos(y_segment * PI);
        float z = std::sin(x_segment * 2.0f * PI) * std::sin(y_segment * PI);
        Vertex<float> vertex;
        vertex.position = Vec3<float>{x, y, z} * radius;
        vertex.normal = Vec3<float>{x, y, z} * radius;
        vertices.emplace_back(vertex);

        if (j < Y_SEGMENTS - 1) {
          const int curRow  = j * X_SEGMENTS;
          const int nextRow = (j + 1) * X_SEGMENTS;
          const int nextS   = (i + 1) % X_SEGMENTS;

          indices.push_back(curRow + i);
          indices.push_back(nextRow + i);
          indices.push_back(nextRow + nextS);

          indices.push_back(curRow + i);
          indices.push_back(nextRow + nextS);
          indices.push_back(curRow + nextS);
        }
      }
    }
  }
};

// TODO: Use it, remove Primitive::quad
/// Fullscreen quad in NDC
struct Quad: public Mesh {
  Quad(): Mesh() {
    Vertex<float> a;
    a.position = {-1.0f, -1.0f, 1.0f};
    a.texCoord = {0.0f, 1.0f};
    Vertex<float> b;
    b.position = {-1.0f, -1.0f, 0.0f};
    b.texCoord = {0.0f, 0.0f};
    Vertex<float> c;
    c.position = {1.0f,  1.0f, 0.0f};
    c.texCoord = {1.0f, 1.0f};
    Vertex<float> d;
    d.position = {1.0f, -1.0f, 0.0f};
    d.texCoord = {1.0f, 0.0f};
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
  Vec3<T> normal() const {
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

struct GraphicsState {
  ShadingModel shading_model = ShadingModel::Unlit;
  ID mesh_id; // TODO: Seperate this from the GState 
  Texture diffuse_texture;
  Texture metallic_roughness_texture; // Used by ShadingModel::PBRTextured
  Texture ambient_occlusion_texture;
  Texture emissive_texture;
  Vec3<float> position;
  float scale = 1.0;
  Vec3<float> pbr_scalar_parameters; // Used by ShadingModel::PBRScalars
};

/// Represents the state of the Render, used for ImGUI debug panes
struct RenderState {
  uint64_t entities;
  uint64_t graphic_batches;
};

using Vec2f = Vec2<float>;
using Vec3f = Vec3<float>;
using Vec4f = Vec4<float>;
using Mat4f = Mat4<float>;

#endif // MEINEKRAFT_PRIMITIVES_H
