#pragma once
#ifndef MEINEKRAFT_RENDERPASS_HPP
#define MEINEKRAFT_RENDERPASS_HPP

#include "shader.hpp"
#include "../math/vector.hpp"

#include <stdint.h>

struct Renderer;

class RenderPass {
public:
  const char* name = nullptr; // Descriptive name of the RenderPass
  Shader shader;
  // Global buffers needs to be accessable from the Renderer
  virtual bool setup(Renderer& renderer) = 0;
  // virtual bool screen_changed(Renderer& renderer) = 0; 
  virtual bool render() const = 0;
  virtual bool teardown() = 0;
};

class BlurPass: public RenderPass {
  const char* name = "Blur pass";

  uint32_t gl_blur_fbo = 0;
  uint32_t gl_blur_texture_unit = 0;
  uint32_t gl_blur_texture = 0;
  uint32_t gl_blur_vao = 0;
  // Renderer Interface
  bool setup(Renderer& renderer) override final;
  bool render() const override final;
};

class SSAOPass: public RenderPass {
  const char* name = "SSAO pass";

  uint32_t ssao_num_samples = 64;
  float ssao_kernel_radius = 1.0f;
  float ssao_power = 1.0f;
  float ssao_bias = 0.0025f;
  float ssao_blur_factor = 16.0f;
  bool  ssao_blur_enabled = false;

  std::vector<Vec3f> ssao_samples;

  uint32_t gl_ssao_fbo = 0;
  uint32_t gl_ssao_texture = 0;
  uint32_t gl_ssao_texture_unit = 0;
  uint32_t gl_ssao_vao = 0;

  uint32_t gl_ssao_noise_texture = 0;
  uint32_t gl_ssao_noise_texture_unit = 0;

  // Renderer Interface
  bool setup(Renderer& renderer) override final;
  bool render() const override final;
};

class LightingPass : public RenderPass {
  const char* name = "Lighting pass";

  // Renderer Interface
  bool setup(Renderer& renderer) override final;
  bool render() const override final;
};

#endif // MEINEKRAFT_RENDERPASS_HPP