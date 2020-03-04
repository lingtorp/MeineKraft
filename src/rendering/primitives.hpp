#pragma once
#ifndef MEINEKRAFT_PRIMITIVES_HPP
#define MEINEKRAFT_PRIMITIVES_HPP

#include <vector>
#include <array>
#include <algorithm>

#include "../math/vector.hpp"

#include <glm/glm.hpp>
#include <glm/common.hpp>

/// Mathematical constants
constexpr double PI  = 3.1415926535897932384626433832795;
constexpr float PI_F = 3.141592653589f;

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
  Vec3f tangent   = {};
  Vertex() = default;
  explicit Vertex(const Vec3f position): position(position), tex_coord{}, normal{}, tangent{} {};
  Vertex(const Vec3f position, const Vec2f tex_coord): position(position), tex_coord(tex_coord), normal{}, tangent{} {};

  bool operator==(const Vertex& rhs) const {
      return position == rhs.position && tex_coord == rhs.tex_coord && normal == rhs.normal && tangent == rhs.tangent;
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
          auto hashed_tangent_x = hasher(vertex.tangent.x);
          auto hashed_tangent_y = hasher(vertex.tangent.y);
          auto hashed_tangent_z = hasher(vertex.tangent.z);

          size_t seed = 0;
          hash_combine(seed, hashed_x);
          hash_combine(seed, hashed_y);
          hash_combine(seed, hashed_z);
          hash_combine(seed, hashed_texcoord_x);
          hash_combine(seed, hashed_texcoord_y);
          hash_combine(seed, hashed_normal_x);
          hash_combine(seed, hashed_normal_y);
          hash_combine(seed, hashed_normal_z);
          hash_combine(seed, hashed_tangent_x);
          hash_combine(seed, hashed_tangent_y);
          hash_combine(seed, hashed_tangent_z);
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
  std::vector<Vertex> vertices  = {};
  std::vector<uint32_t> indices = {};

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
        const float x = mk_cosf(x_segment * 2.0f * PI_F) * mk_sinf(y_segment * PI_F);
        const float y = mk_cosf(y_segment * PI_F);
        const float z = mk_sinf(x_segment * 2.0f * PI_F) * mk_sinf(y_segment * PI_F);
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
    a.position = {-1.0f, -1.0f, 0.0f};
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
      return a * point.x + b * point.y + c * point.z + d;
  }

  /// Normalizes the plane's equation and returns it
  static inline Vec4f normalize(const Vec4f& p) {
    const float mag = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    return Vec4f(p.x / mag, p.y / mag, p.z / mag, p.w / mag);
  }
};

/// Opaque ID type used to reference resources throughout the engine
typedef uint64_t ID;

enum class ShadingModel: uint32_t {
  Unlit = 1,                        // Unlit, using its surface color 
  PhysicallyBased = 2,              // PBR using textures (default)
  PhysicallyBasedScalars = 3        // PBR using scalars instead of textures
};

enum class ShadowAlgorithm : uint8_t {
  Plain = 0,                     // Normal shadowmapping
  PercentageCloserFiltering = 1, // PCF shadowing
  VCT = 2                        // Voxel cone tracing based shadowing
};

/// Represents the state of the Render system, used for ImGUI debug panes
/// NOTE: execution_time per render pass is NOT stable frame to frame after a render pass has changed, the pipeline needs to be static to be accurate
struct RenderState {
  // General renderer system information
  uint64_t frame           = 0;
  uint32_t entities        = 0;
  uint32_t graphic_batches = 0;
  uint32_t render_passes   = 0;      // Number of Renderpasses executed this frame
  static const uint32_t execution_time_buffer_size = 5; // Size of the buffer for the execution time of each Renderpass
  bool capture_execution_timings = true; // Enables querying for render pass execution times, lowers performance
  uint64_t total_execution_time = 0;     // NOTE: for all render passes in ns

  // Global illumination related
  struct {
    // NOTE: Execution time of lighting application only applies radiance does not compute it (hence very fast)
    uint64_t execution_time[execution_time_buffer_size] = {0};      // NOTE: nanoseconds
    bool shadowmapping = true;
    bool normalmapping = true;
    bool indirect = true;
    bool ambient  = true;             // Dependent on 'indirect' when VCT is used as GI algorithm
    bool direct = true;
    bool specular = true;
    bool emissive = true;
    int32_t downsample_modifier = 2;  // GI is performed in downsampled space (1 / modifier) * full_res
  } lighting;

  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
  } gbuffer_downsample;

  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
    bool enabled = true;
  } culling;

  // GBuffer generation pass (a.k.a geometry pass)
  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
  } gbuffer;

  // Voxelization related (used by VCT pass)
  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
    bool always_voxelize = true;
    bool voxelize = true;        // NOTE: Toggled by the Renderer (a.k.a executed once)
    bool conservative_rasterization = false;
  } voxelization;

  // Voxel cone tracing related
  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
    const uint32_t MAX_DIFFUSE_CONES = 12;     // FIXME: Diffuse cone hardcoded limit
    float roughness_aperature = 60.0f;         // 60 deg diffuse cone from [Rauwendaal, Crassin11]
    float metallic_aperature   = 0.1f;         // 10 deg specular cone from [Crassin11]
    int num_diffuse_cones = 6;                 // [Crassin11], [Yeu13] suggests 5
    float specular_cone_trace_distance = 0.25f;// Specular cone trace distance in terms of factor of max scene length
    float ambient_decay = 0.2f;                // [Crassin11] mentions but does not specify decay factor for scene ambient
  } vct;

  // Direct/shadows related
  struct {
    uint64_t execution_time_shadowmapping[execution_time_buffer_size] = {0}; // NOTE: nanoseconds, generating the depth map
    uint64_t execution_time_shadow[execution_time_buffer_size] = {0};        // NOTE: nanoseconds, applying direct shadows
    bool enabled = true;                                // Enable direct lighting computation
    ShadowAlgorithm algorithm = ShadowAlgorithm::Plain; // See enum class ShadowAlgorithm
    float bias = 0.00025f;                              // Shadow bias along geometric normal of surface
    int32_t pcf_samples = 2;                            // Number of depth samples taken with PCF
    float vct_cone_aperature = 0.0050f;                 // Shadow cone aperature
    int32_t shadowmap_resolution_step = 2;              // TODO: Hook up and use
    const uint32_t SHADOWMAP_W = 2 * 2048;              // Shadowmap texture width
    const uint32_t SHADOWMAP_H = SHADOWMAP_W;           // Shadowmap texture height
  } shadow;

  // Bilateral filtering related
  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
    bool enabled = true;                // Bilateral filtering pass to filter the radiance
    bool direct = false;                // Enable filtering of the direct radiance
    bool ambient = true;                // Enable filtering of the ambient radiance
    bool indirect = true;               // Enable filtering of the indirect radiance
    bool specular = true;               // Enable filtering of the specular radiance
    bool pixel_diff = false;            // Toggled. Saves pre/post screenshot of the currently enabled filters
    std::vector<float> kernel;          // Spatial kernel values generated from sigma and radius
    float spatial_kernel_sigma = 0.8f;  // Sigma of the spatial upsampling kernel in texture space
    uint32_t spatial_kernel_radius = 5; // Radius of the spatial upsampling kernel in texture space
    bool position_weight = true;        // Enable position as a weight in filtering
    float position_sigma = 2.0f;        // FIXME: How to set this value or tune it?
    bool normal_weight = false;         // Enable normals as a weight in filtering
    float normal_sigma = 2.0f;          // FIXME: How to set this value or tune it?
    bool depth_weight = false;          // Enable depth as a weight in filtering
    float depth_sigma = 2.0f;           // FIXME: How to set this value or tune it?
    bool normalmapping = false;         // Enable displacement of normals when normals used as weight in filtering
  } bilateral_filtering;

  // Bilateral upsampling related
  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
    bool enabled = true;               // Bilateral upsampling pass based on the bilateral_filtering states
    bool ambient = true;
    bool indirect = true;
    bool specular = true;
    bool depth_weight = true;          // Depth guidance texture used to upsample image
    bool normal_weight = false;        // Normal guidance texture used to upsample image
    bool position_weight = false;      // Position guidance texture used to upsample image
    bool normal_mapping = false;
  } bilateral_upsample;

  // Bilinear upsampling related
  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
    bool enabled = false;
    bool nearest_neighbor = false;
    bool ambient = true;
    bool indirect = true;
    bool specular = true;
  } bilinear_upsample;

  // Voxel visualization related
  // FIXME: Voxel visualization does not work ...
  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
    bool enabled = false;
  } voxel_visualization;

  // Final blit pass
  struct {
    uint64_t execution_time[execution_time_buffer_size] = {0}; // NOTE: nanoseconds
  } blit;

  RenderState() = default;

  friend std::ostream &operator<<(std::ostream &os, const RenderState& state) {
    // TODO: Implement printable struct RenderState
    return os;
  }
};

struct Resolution {
  int width, height;
};

static auto HD      = Resolution{1280, 720};
static auto FULL_HD = Resolution{1920, 1080};

struct DirectionalLight {
  Vec3f intensity;
  Vec3f direction;
  // TODO: Find a typical Directional light intensity
  DirectionalLight(const Vec3f& direction): direction(direction), intensity(Vec3f(1.0f)) {}

  friend std::ostream &operator<<(std::ostream &os, const DirectionalLight& l) {
    return os << "DirectionalLight(direction: " << l.direction << ")";
  }
};

struct AABB {
  float scaling_factor = 1.0f;
	Vec3f max;
	Vec3f min;
	AABB() = default;
	AABB(const Vec3f& min, const Vec3f& max): max(max), min(min) {}

	// Along the diagonal (from min to max) of the AABB
	inline float diagonal_lng() const { return (max - min).length(); }
	// Along x-axis
	inline float width() const { return std::abs(max.x - min.x); }
	// Along y-axis
	inline float height() const { return std::abs(max.y - min.y); }
	// Along z-axis
	inline float breadth() const { return std::abs(max.z - min.z); }

  /// Returns true if all the sides are equal
	inline bool is_cubic() const { return width() == height() && height() == breadth(); }

  /// Returns the midpoint/center of the AABB
	inline Vec3f center() const { return min + ((max - min) / 2.0f); }

  /// Returns the magnitude of the largest axis of the AABB
  inline float max_axis() const {
    const std::array<float, 3> axis = {width(), height(), breadth()};
    return *std::max_element(axis.begin(), axis.end());
  }

	friend std::ostream& operator<<(std::ostream& os, const AABB& aabb) {
		return os << "AABB(min: " << aabb.min << ", max: " << aabb.max << ", scale: " << aabb.scaling_factor << ")";
	}
};

#endif // MEINEKRAFT_PRIMITIVES_HPP
