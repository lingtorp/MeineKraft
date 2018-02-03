#include <SDL2/SDL_log.h>
#include <SDL_opengl.h> // TODO: Remove.
#include "rendercomponent.h"
#include "render.h"
#include "shader.h"
#include "../nodes/entity.h"
#include "meshmanager.h"
#include "../util/filesystem.h"

RenderComponent::RenderComponent(Entity* entity): entity(entity), graphics_state{} {}

void RenderComponent::set_mesh(const std::string& directory, const std::string& file) {
  // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
  ID mesh_id;
  std::vector<std::pair<Texture::Type, std::string>> texture_info;
  std::tie(mesh_id, texture_info) = Renderer::instance().mesh_manager->load_mesh(directory, file);
  graphics_state.mesh_id = mesh_id;
  for (const auto& pair : texture_info) {
    auto texture_type = pair.first;
    auto texture_file = pair.second;
    switch (texture_type) {
      case Texture::Type::Diffuse:
        graphics_state.diffuse_texture.resource = TextureResource{texture_file};
        graphics_state.diffuse_texture.gl_texture_type = GL_TEXTURE_2D_ARRAY; // FIXME: Assumes texture format
        graphics_state.diffuse_texture.used = true;
        graphics_state.diffuse_texture.id = graphics_state.diffuse_texture.resource.to_hash();
        break;
      case Texture::Type::Specular:
        break;
      default:
        exit(1);
    }
  }
};

void RenderComponent::update() {
  graphics_state.scale = entity->scale;
  graphics_state.position = entity->position;
}

void RenderComponent::set_mesh(MeshPrimitive primitive) {
  // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
  graphics_state.mesh_id = Renderer::instance().mesh_manager->mesh_id_from_primitive(primitive);
}

void RenderComponent::set_cube_map_texture(const std::vector<std::string>& faces) {
  TextureResource texture_resource;
  texture_resource.files = faces;
  graphics_state.diffuse_texture.resource = texture_resource;
  graphics_state.diffuse_texture.gl_texture_type = GL_TEXTURE_CUBE_MAP_ARRAY;
  graphics_state.diffuse_texture.used = true;
  graphics_state.diffuse_texture.id = graphics_state.diffuse_texture.resource.to_hash();
}

/// RenderComponents are not supposed to be modified and only re-created
void RenderComponent::did_attach_to_entity(Entity* entity) {
  Renderer::instance().add_to_batch(this);
}
