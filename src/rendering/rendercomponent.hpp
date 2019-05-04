#pragma once
#ifndef MEINEKRAFT_RENDERCOMPONENT_HPP
#define MEINEKRAFT_RENDERCOMPONENT_HPP

#include "primitives.hpp"
#include "texture.hpp"

struct RenderComponent {
  ShadingModel shading_model = ShadingModel::Unlit;
  ID mesh_id; 
  Texture diffuse_texture;            // ?
  Texture metallic_roughness_texture; // Used by ShadingModel::PBRTextured
  Texture ambient_occlusion_texture;  // ?
  Texture emissive_texture;           // ?
  Texture normal_texture;             // Tangent space normal map (used by all shading models)
  Vec3f pbr_scalar_parameters;        // Used by ShadingModel::PBRScalars (r,g,b) = (unused, roughness, metallic)

  /// Tries to set the mesh for the RenderComponent from the .obj file in directory_file
  /// Loads and sets the relevant textures from the loaded material in the model
  void set_mesh(const std::string& directory, const std::string& file);

  /// Loads all meshes in a file and returns multiple RenderComponents
  static std::vector<RenderComponent> load_scene_models(const std::string& directory, const std::string& file);

  /// Sets the cube map texture to the bounded mesh
  /// order; right, left, top, bot, back, front
  void set_cube_map_texture(const std::vector<std::string>& faces);

  /** Helper methods mainly */
  void set_mesh(MeshPrimitive primitive) {
    this->mesh_id = ID(primitive);
  }

  void set_shading_model(const ShadingModel shading_model) {
    this->shading_model = shading_model;
  }
};

#endif // MEINEKRAFT_RENDERCOMPONENT_HPP
