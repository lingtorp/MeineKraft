#include "model.h"

Model::Model(std::string mesh_file, std::string directory): Entity() {
  auto render_comp = new RenderComponent(this);
  render_comp->set_mesh(mesh_file, directory);
  render_comp->enable_shading();
  attach_component(render_comp);
  scale = 1;
}
