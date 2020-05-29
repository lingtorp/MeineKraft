#pragma once

#include "primitives.hpp"
#include "rendercomponent.hpp"

struct Scene;

struct RenderSystem {

  /// Create a RenderSystem with a given window/screen size/resolution
  RenderSystem(const Resolution& screen);
  ~RenderSystem();

  /// Post allocation initialization called at engine startup
  bool init();

  /// Renders one frame function using the Renderstate in 'state'
  void render_frame();

  /// Adds the data of a RenderComponent to a internal batch
  void add_component(const RenderComponent comp, const ID entity_id);

  /// Removes the RenderComponent associated with the CID if there exists one
  void remove_component(const ID entity_id);

  Scene* scene = nullptr;
  RenderState state;
  Resolution screen;
};
