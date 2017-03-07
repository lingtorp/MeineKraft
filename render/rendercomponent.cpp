#include "rendercomponent.h"
#include "render.h"
#include "../nodes/entity.h"

RenderComponent::RenderComponent(Entity *entity): entity(entity), graphics_state{} {}

void RenderComponent::set_obj_mesh(std::string mesh_file, std::string directory_file) {
    // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
    auto mesh_id = Renderer::instance().load_mesh(mesh_file, directory_file);
    Renderer::instance().add_to_batch(this, mesh_id);
    Renderer::instance().load_obj_textures(this, mesh_file, directory_file);
    // TODO: Need to change so that not every texture is loaded multiple times, TextureManager should coordinate it
};

void RenderComponent::update() {
    graphics_state.scale = entity->scale;
    graphics_state.position = entity->position;
}

void RenderComponent::set_mesh(MeshPrimitive primitive) {
    // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
    auto mesh_id = Renderer::instance().load_mesh_primitive(primitive);
    Renderer::instance().add_to_batch(this, mesh_id);
}

void RenderComponent::set_cube_map_texture(std::vector<std::string> faces) {
    auto texture = Texture();
    texture.load_cube_map(faces);
    graphics_state.diffuse_texture = Renderer::instance().setup_texture(this, texture);
};
