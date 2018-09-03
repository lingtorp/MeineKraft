#ifndef MEINEKRAFT_RENDER_H
#define MEINEKRAFT_RENDER_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <memory>

#include "texture.h"
#include "light.h"
#include "transform.h"
#include "shader.h"

#ifdef _WIN32
#include <SDL_video.h>
#else
#include <sdl2/SDL_video.h>
#endif 

struct World;
class Camera;
class RenderComponent;
class GraphicsBatch;
class Shader;
class FileMonitor;
struct MeshManager;

struct ShaderBinding {
  std::string gl_name;
  int value_type;
};

class RenderPass {
  ShaderBinding* bindings;
  virtual bool setup() const = 0;
  virtual bool start() const = 0;
  virtual bool end() const = 0;
  virtual bool teardown() const = 0;
};

class SSAOPass : RenderPass {
  /// SSAO
  uint32_t ssao_num_samples = 64;
  float ssao_kernel_radius = 1.0f;
  float ssao_power = 1.0f;
  float ssao_bias = 0.0025;
  float ssao_blur_factor = 16.0f;
  bool  ssao_blur_enabled = false;
};

class RenderPipeline {
  RenderPass* passes;
};

class Renderer {
public:
  Renderer(Renderer &render) = delete;
  ~Renderer();

  /// Singleton instance of core Render, use with caution.
  static Renderer &instance() {
    static Renderer instance;
    return instance;
  }

  /// Main render function, renders all the graphics batches and so on
  void render(uint32_t delta);
  
  /// Adds the RenderComponent to a internal batch
  uint64_t add_to_batch(RenderComponent* comp);

  /// Updates all the shaders projection matrices in order to support resizing of the window
  void update_projection_matrix(float fov);
  
  Camera* camera;
  RenderState state;
  SDL_Window *window;
  MeshManager* mesh_manager;
  float draw_distance = 200;
  Mat4<float> projection_matrix;
  float screen_width;
  float screen_height;
  
  /// SSAO
  uint32_t ssao_num_samples = 64;
  float ssao_kernel_radius = 1.0f;
  float ssao_power = 1.0f;
  float ssao_bias = 0.0025;
  float ssao_blur_factor = 16.0f;
  bool  ssao_blur_enabled = false;
  
  std::vector<PointLight> lights;
  bool lightning_enabled = true;
  bool animate_light = true;
  float specular_power = 32.0;
  bool blinn_phong_shading = true;
private:
  Renderer();
  
  /// Geometry pass related
  Shader* depth_shader;
  uint32_t gl_depth_fbo;
  uint32_t gl_depth_texture;
  uint32_t gl_depth_texture_unit;
  
  /// SSAO pass related
  Shader* ssao_shader;
  uint32_t gl_ssao_fbo;
  uint32_t gl_ssao_texture;
  uint32_t gl_ssao_texture_unit;
  
  uint32_t gl_ssao_noise_texture;
  uint32_t gl_ssao_noise_texture_unit;
  
  std::vector<Vec3<float>> ssao_samples;
  
  /// Blur pass related
  Shader* blur_shader;
  uint32_t gl_blur_fbo;
  uint32_t gl_blur_texture;
  uint32_t gl_blur_texture_unit;
  
  /// Lightning pass related
  Shader* lightning_shader;
  // Point light
  uint32_t gl_pointlight_models_buffer_object;
  // Used since default fbo is not to be trusted
  uint32_t gl_lightning_texture;
  uint32_t gl_lightning_fbo;
  uint32_t gl_lightning_texture_unit;
  
  /// Global buffers
  // Normals
  uint32_t gl_normal_texture;
  uint32_t gl_normal_texture_unit;
  // Positions
  uint32_t gl_position_texture;
  uint32_t gl_position_texture_unit;
  // Albedo
  uint32_t gl_albedo_texture;
  uint32_t gl_albedo_texture_unit;
  
  uint32_t gl_lightning_vao;
  uint32_t gl_blur_vao;
  uint32_t gl_ssao_vao;
  
  std::vector<Transform> transformations;

  std::vector<GraphicsBatch> graphics_batches;

  FileMonitor* shader_file_monitor;

  // TODO Docs
  bool point_inside_frustrum(Vec3<float> point, std::array<Plane<float>, 6> planes);

  // TODO Docs
  std::array<Plane<float>, 6> extract_planes(Mat4<float> matrix);

  /// Setups the VAO and uniforms up between the batch and OpenGL
  void link_batch(GraphicsBatch &batch);

  /// Creates a camera view matrix based on the euler angles (x, y) and position of the eye
  Mat4<float> FPSViewRH(Vec3<float> eye, float pitch, float yaw);
};

#endif // MEINEKRAFT_RENDER_H
