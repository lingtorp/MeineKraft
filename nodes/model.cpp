#include "model.h"

Model::Model(const std::string& directory, const std::string& file) {
  RenderComponent render;
  render.set_mesh(directory, file);
  render.set_shading_model(ShadingModel::PhysicallyBased);
  attach_component(render);
}
