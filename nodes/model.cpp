#include "model.h"

Model::Model(const std::string& directory, const std::string& file) {
  NameSystem::instance().add_name_to_entity(file, id);
  TransformComponent transform;
  transform.position = Vec3f(-2.0f, 2.0f, 0.0f);
  transform.scale = 1.0f;
  attach_component(transform);
  RenderComponent render;
  render.set_mesh(directory, file);
  render.set_shading_model(ShadingModel::PhysicallyBased);
  attach_component(render);
}
