#include <SDL2/SDL_log.h>
#include "rendercomponent.h"
#include "render.h"
#include "../nodes/entity.h"

RenderComponent::RenderComponent(Entity *entity): entity(entity), graphics_state{} {}

void RenderComponent::set_mesh(std::string mesh_file, std::string directory_file) {
    // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
    graphics_state.mesh_id = Renderer::instance().load_mesh(this, mesh_file, directory_file);
};

void RenderComponent::update() {
    graphics_state.scale = entity->scale;
    graphics_state.position = entity->position;
}

void RenderComponent::set_mesh(MeshPrimitive primitive) {
    // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
    graphics_state.mesh_id = Renderer::instance().load_mesh_primitive(primitive, this);
}

void RenderComponent::set_cube_map_texture(std::vector<std::string> faces) {
    auto texture = Texture();
    auto success = texture.load_cube_map(faces);
    if (!success) { SDL_Log("RenderComponent: Failed to load cube map"); }
    graphics_state.diffuse_texture = Renderer::instance().setup_texture(this, texture);
};
