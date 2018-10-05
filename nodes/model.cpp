#include "model.h"

Model::Model(std::string directory, std::string file) {
  auto render_comp = new RenderComponent(this);
  render_comp->set_mesh(directory, file);
  render_comp->set_shading_model(ShadingModel::PhysicallyBased);
  attach_component(render_comp);
}
