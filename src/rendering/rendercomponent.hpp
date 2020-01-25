#pragma once
#ifndef MEINEKRAFT_RENDERCOMPONENT_HPP
#define MEINEKRAFT_RENDERCOMPONENT_HPP

#include "primitives.hpp"
#include "texture.hpp"

/// RenderComponents is the visual representation of a Entity.
/// All of the configurations that are supported by the engine is provided
/// with this struct and attached to a Entity. The Renderer will then determine
/// how to setup the shaders and try to batch the RenderComponent in a batch
/// based on the shader configuration required and the geometry (mesh).
struct RenderComponent {
  ShadingModel shading_model = ShadingModel::Unlit;
  ID mesh_id = 0; 
  Texture diffuse_texture;            // ? FIXME: Who takes ownership of the texture?
  Texture metallic_roughness_texture; // Used by ShadingModel::PBRTextured
  Texture ambient_occlusion_texture;  // ? FIXME: Who takes ownership of the texture?
  Texture emissive_texture;           // ? FIXME: Who takes ownership of the texture?
  Texture normal_texture;             // Tangent space normal map (used by all shading models) FIXME: Who takes ownership of the texture?
  Vec3f pbr_scalar_parameters;        // Used by ShadingModel::PBRScalars (r,g,b) = (unused, roughness, metallic)
  Vec3f emissive_scalars;             // Emissive scalars instead of texture
  Vec3f diffuse_scalars;              // Diffuse scalars instead of texture

  /// Tries to set the mesh for the RenderComponent from the .obj file in directory_file
  /// Loads and sets the relevant textures from the loaded material in the model
  void set_mesh(const std::string& directory, const std::string& file);

  /// Loads all meshes in a file and returns multiple RenderComponents
  static std::vector<RenderComponent> load_scene_models(const std::string& directory, const std::string& file);

  /// Sets the cube map texture to the bounded mesh
  /// order; right, left, top, bot, back, front
  void set_cube_map_texture(const std::vector<std::string>& faces);

  /// Sets the emissive color of the object (uses texture if defined)
  void set_emissive_color(const Vec3f& color) {
    this->emissive_scalars = color;
  }

  /// Sets the diffuse color of the object (uses texture if defined)
  void set_diffuse_color(const Vec3f& color) {
    this->diffuse_scalars = color;
  }

  /** Helper methods mainly */
  void set_mesh(MeshPrimitive primitive) {
    this->mesh_id = ID(primitive);
  }

  void set_shading_model(const ShadingModel shading_model) {
    this->shading_model = shading_model;
  }
};

#endif // MEINEKRAFT_RENDERCOMPONENT_HPP
