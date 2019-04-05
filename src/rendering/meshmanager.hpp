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
  static std::pair<ID, std::vector<std::pair<Texture::Type, std::string>>>
  load_mesh(const std::string& directory, const std::string& file);
  
  static Mesh mesh_from_id(ID id);
};

#endif // MEINEKRAFT_MESHMANAGER_HPP
