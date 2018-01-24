#include <SDL2/SDL_log.h>
#include <SDL_opengl.h> // TODO: Remove.
#include "rendercomponent.h"
#include "render.h"
#include "shader.h"
#include "../nodes/entity.h"
#include "meshmanager.h"
#include "../util/filesystem.h"

RenderComponent::RenderComponent(Entity* entity): entity(entity), graphics_state{} {
  graphics_state.shading = true;
}

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
}

/// RenderComponents are not supposed to be modified and only re-created
void RenderComponent::did_attach_to_entity(Entity* entity) {
  // 1. Generate a Shader
  /// Compile shader
  const auto vertex_shader   = FileSystem::base + "shaders/std/vertex-shader.glsl";
  const auto fragment_shader = FileSystem::base + "shaders/std/fragment-shader.glsl";
  Shader shader(vertex_shader, fragment_shader);
  if (graphics_state.shading) {
    shader.add("#define FLAG_BLINN_PHONG_SHADING \n");
  }
  
  if (graphics_state.diffuse_texture.used) {
    if (graphics_state.diffuse_texture.gl_texture_type == GL_TEXTURE_CUBE_MAP_ARRAY) {
      shader.add("#define FLAG_CUBE_MAP_TEXTURE \n");
    }
    if (graphics_state.diffuse_texture.gl_texture_type == GL_TEXTURE_2D_ARRAY) {
      shader.add("#define FLAG_2D_TEXTURE \n");
    }
  }
  
  std::string err_msg;
  bool success;
  std::tie(success, err_msg) = shader.compile();
  if (!success) {
    SDL_Log("Shader compilation failed; %s", err_msg.c_str());
    return;
  }
  
  // 2. Connect Shader with the textures
  if (graphics_state.diffuse_texture.used) {
    graphics_state.diffuse_texture.gl_texture_location = shader.get_uniform_location(Texture::Type::Diffuse);
    graphics_state.diffuse_texture.id = graphics_state.diffuse_texture.resource.to_hash();
  }
  
  // 5. Add to batch
  graphics_state.batch_id = Renderer::instance().add_to_batch(this, shader);
}

void RenderComponent::enable_shading() {
  graphics_state.shading = true;
};
