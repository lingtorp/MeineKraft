#ifndef VIEW_FRUSTUM_CULLING_RENDERPASS
#define VIEW_FRUSTUM_CULLING_RENDERPASS

#include "renderpass.hpp"

struct ComputeShader;

struct ViewFrustumCullingRenderPass: public RenderPass {

  ComputeShader* shader = nullptr;

  virtual bool setup(Renderer* render);
  virtual bool render(Renderer* render);
};

#endif // VIEW_FRUSTUM_CULLING_RENDERPASS
