#include "render_system.hpp"


/// Create a RenderSystem with a given window/screen size/resolution
RenderSystem::RenderSystem(const Resolution& screen) {

}

RenderSystem::~RenderSystem() {

}

/// Post allocation initialization called at engine startup
bool RenderSystem::init() {


  return true;
}

/// Renders one frame function using the Renderstate in 'state'
void RenderSystem::render_frame() {

}

/// Adds the data of a RenderComponent to a internal batch
void RenderSystem::add_component(const RenderComponent comp, const ID entity_id) {

}

/// Removes the RenderComponent associated with the CID if there exists one
void RenderSystem::remove_component(const ID cid) {
  // TODO
}
