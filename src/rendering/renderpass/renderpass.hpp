#pragma once
#ifndef MEINEKRAFT_RENDERPASS_HPP
#define MEINEKRAFT_RENDERPASS_HPP

// TODO: Docs

struct Renderer;

// TODO: Rename to IRenderPass
class RenderPass {
public:
  virtual bool setup(Renderer* render) = 0;
  virtual bool render(Renderer* render) = 0;
};

#endif // MEINEKRAFT_RENDERPASS_HPP
