#include "model.hpp"

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

Model::Model(const RenderComponent& render) {
  TransformComponent transform;
  attach_component(transform);
  attach_component(render);
}

Scene::Scene(const std::string& directory, const std::string& file) {
  std::vector<RenderComponent> render_components = RenderComponent::load_scene_models(directory, file);
  for (size_t i = 0; i < render_components.size(); i++) {
    render_components[i].set_shading_model(ShadingModel::PhysicallyBased);
    Model model(render_components[i]);
    NameSystem::instance().add_name_to_entity("mesh-" + std::to_string(i), model.id);
  }
}
