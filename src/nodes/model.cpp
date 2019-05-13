#include "model.hpp"
#include "../rendering/meshmanager.hpp"

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

static AABB compute_AABB_from(const RenderComponent& render_component) {
	return AABB();
}

static AABB compute_aabb_from(const std::vector<RenderComponent>& render_components) {
	AABB aabb;
	aabb.min = Vec3f(std::numeric_limits<float>::min());
	aabb.max = Vec3f(std::numeric_limits<float>::min());
	for (size_t i = 0; i < render_components.size(); i++) {
		Vec3f max = Vec3f(std::numeric_limits<float>::min());
		Vec3f min = Vec3f(std::numeric_limits<float>::max());
		const Mesh mesh = MeshManager::mesh_from_id(render_components[i].mesh_id);
		for (size_t j = 0; j < mesh.vertices.size(); j++) {
			const Vertex& vertex = mesh.vertices[j];
			if (max.x < vertex.position.x) { max.x = vertex.position.x; }
			if (max.y < vertex.position.y) { max.y = vertex.position.y; }
			if (max.z < vertex.position.z) { max.z = vertex.position.z; }
			if (vertex.position.x < min.x) { min.x = vertex.position.x; }
			if (vertex.position.y < min.y) { min.y = vertex.position.y; }
			if (vertex.position.z < min.z) { min.z = vertex.position.z; }
		}
		if (aabb.max.length() < max.length()) { aabb.max = max; }
		if (aabb.min.length() < min.length()) { aabb.min = min; }
	}
	Log::info(aabb);
	return aabb;
}

Scene::Scene(const std::string& directory, const std::string& file) {
  std::vector<RenderComponent> render_components = RenderComponent::load_scene_models(directory, file);
	aabb = compute_aabb_from(render_components);
  for (size_t i = 0; i < render_components.size(); i++) {
    render_components[i].set_shading_model(ShadingModel::PhysicallyBased);
    Model model(render_components[i]);
    NameSystem::instance().add_name_to_entity("mesh-" + std::to_string(i), model.id);
  }
}
