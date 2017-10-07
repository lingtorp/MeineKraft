#include "model.h"

Model::Model(std::string directory, std::string file) {
  auto render_comp = new RenderComponent(this);
  render_comp->set_mesh(directory, file);
  render_comp->enable_shading();
  attach_component(render_comp);
  scale = 1;
}
