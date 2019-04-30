#pragma once
#ifndef MEINEKRAFT_MESHMANAGER_HPP
#define MEINEKRAFT_MESHMANAGER_HPP

#include "primitives.hpp"
#include "texture.hpp"

#include <vector>

struct MeshInformation {
    Mesh mesh;
    std::string loaded_from_filepath;
};

struct MeshManager {
  // Loads the root node of a model file
  static std::pair<ID, std::vector<std::pair<Texture::Type, std::string>>>
  load_mesh(const std::string& directory, const std::string& file);

  // Loads all the meshs and materials in a model file
  static std::pair<std::vector<ID>, std::vector<std::vector<std::pair<Texture::Type, std::string>>>>
  load_meshes(const std::string& directory, const std::string& file);
  
  static Mesh mesh_from_id(ID id);
};

#endif // MEINEKRAFT_MESHMANAGER_HPP
