#ifndef MEINEKRAFT_RENDERCOMPONENT_H
#define MEINEKRAFT_RENDERCOMPONENT_H

#include "primitives.h"

struct RenderComponent {
  ShadingModel shading_model = ShadingModel::Unlit;
  ID mesh_id; 
  Texture diffuse_texture;
  Texture metallic_roughness_texture; // Used by ShadingModel::PBRTextured
  Texture ambient_occlusion_texture;
  Texture emissive_texture;
  Vec3f pbr_scalar_parameters;        // Used by ShadingModel::PBRScalars

  /// Sets the mesh for the RenderComponent from the .obj file in directory_file
  void set_mesh(const std::string& directory, const std::string& file);

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

#endif // MEINEKRAFT_RENDERCOMPONENT_H
