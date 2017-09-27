#include <SDL2/SDL_log.h>
#include <SDL_opengl.h> // TODO: Remove.
#include "rendercomponent.h"
#include "render.h"
#include "../nodes/entity.h"
#include "meshmanager.h"

RenderComponent::RenderComponent(Entity *entity): entity(entity), graphics_state{} {}

void RenderComponent::set_mesh(const std::string& mesh_file, const std::string& directory_file) {
  // TODO: Remove from previous batch - since we are changing mesh and thus geo. data
  graphics_state.mesh_id = Renderer::instance().load_mesh(this, mesh_file, directory_file);
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
  auto texture = Texture();
  auto success = texture.load_cube_map(faces);
  if (!success) { SDL_Log("RenderComponent: Failed to load cube map"); }
  graphics_state.diffuse_texture = texture;
}

void RenderComponent::did_attach_to_entity(Entity* entity) {
  // 1. Generate a Shader
  /// Compile shader
  const std::string shader_base_filepath = "/Users/AlexanderLingtorp/Repositories/MeineKraft/shaders/";
  const auto vertex_shader   = shader_base_filepath + "std/vertex-shader.glsl";
  const auto fragment_shader = shader_base_filepath + "std/fragment-shader.glsl";
  Shader shader(vertex_shader, fragment_shader);
  
  if (graphics_state.diffuse_texture.loaded_successfully) {
    if (graphics_state.diffuse_texture.gl_texture_type == GL_TEXTURE_CUBE_MAP) {
      shader.add("#define FLAG_CUBE_MAP_TEXTURE \n");
    }
    if (graphics_state.diffuse_texture.gl_texture_type == GL_TEXTURE_2D) {
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
  
  // 2. Setup the textures
  if (graphics_state.diffuse_texture.loaded_successfully) {
    graphics_state.diffuse_texture.gl_texture_location = shader.get_uniform_location(Texture::Type::Diffuse);
  }
  
  // ?. Add to batch
  graphics_state.batch_id = Renderer::instance().add_to_batch(this, shader);
};
