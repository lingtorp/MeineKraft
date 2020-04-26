#include "model.hpp"
#include "../rendering/meshmanager.hpp"
#include "transform.hpp"

Model::Model(const std::string& directory, const std::string& file) {
  NameSystem::instance().add_name_to_entity(file, id);
  TransformComponent transform;
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

static AABB compute_aabb_from(const RenderComponent& render_component) {
  return compute_aabb_from({render_component});
}

static AABB compute_aabb_from(const std::vector<RenderComponent>& render_components) {
  if (render_components.empty()) {
    Log::error("Tried to compute AABB from a list of zero RenderComponents");
    return AABB();
  }

  AABB aabb;
  aabb.min = Vec3f(std::numeric_limits<float>::max());
  aabb.max = Vec3f(std::numeric_limits<float>::min());
  for (size_t i = 0; i < render_components.size(); i++) {
    Vec3f max = Vec3f(std::numeric_limits<float>::min());
    Vec3f min = Vec3f(std::numeric_limits<float>::max());
    const Mesh& mesh = MeshManager::mesh_from_id(render_components[i].mesh_id);
    for (size_t j = 0; j < mesh.vertices.size(); j++) {
      const Vertex& vertex = mesh.vertices[j];
      if (max.x < vertex.position.x) { max.x = vertex.position.x; }
      if (max.y < vertex.position.y) { max.y = vertex.position.y; }
      if (max.z < vertex.position.z) { max.z = vertex.position.z; }
      if (vertex.position.x < min.x) { min.x = vertex.position.x; }
      if (vertex.position.y < min.y) { min.y = vertex.position.y; }
      if (vertex.position.z < min.z) { min.z = vertex.position.z; }
    }
    if (aabb.max.x < max.x) { aabb.max.x = max.x; }
    if (aabb.max.y < max.y) { aabb.max.y = max.y; }
    if (aabb.max.z < max.z) { aabb.max.z = max.z; }
    if (aabb.min.x > min.x) { aabb.min.x = min.x; }
    if (aabb.min.y > min.y) { aabb.min.y = min.y; }
    if (aabb.min.z > min.z) { aabb.min.z = min.z; }
  }
  return aabb;
}

Scene::Scene(const std::string& directory, const std::string& file) {
  // Time scene loading
  const auto start = std::chrono::high_resolution_clock::now();

  std::vector<RenderComponent> render_components = RenderComponent::load_scene_models(directory, file);
  aabb = compute_aabb_from(render_components);
  for (size_t i = 0; i < render_components.size(); i++) {
    render_components[i].set_shading_model(ShadingModel::PhysicallyBased);
    Model model(render_components[i]);
    NameSystem::instance().add_name_to_entity("mesh-" + std::to_string(i), model.id);
  }

  Log::info_indent(1, aabb);
  Log::info_indent(1, "Center: " + aabb.center().to_string());
  Log::info_indent(1, "Size(dx, dy, dz): " + std::to_string(aabb.width()) + ", " + std::to_string(aabb.height()) + ", " + std::to_string(aabb.breadth()));

  // Root scene Camera default
  const auto position = aabb.center();
  const auto direction = Vec3f(0.0f, 0.0f, 1.0f);
  this->camera = Camera(position, direction);

  const auto end = std::chrono::high_resolution_clock::now();
  const auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
  Log::info_indent(1, "✓ scene loaded in: " + std::to_string(seconds) + " seconds");
}

void Scene::load_models_from(const std::string& directory, const std::string& file) { 
  std::vector<RenderComponent> render_components = RenderComponent::load_scene_models(directory, file);
  for (size_t i = 0; i < render_components.size(); i++) {
    render_components[i].set_shading_model(ShadingModel::PhysicallyBased);
    Model model(render_components[i]);
    NameSystem::instance().add_name_to_entity("2-mesh-" + std::to_string(i), model.id);
  }
}

void Scene::reset_camera() {
  // TODO: Reset to spawned position, direction
  camera.position = aabb.center();
  camera.direction = Vec3f(0.0f, 0.0f, 1.0f);
}
