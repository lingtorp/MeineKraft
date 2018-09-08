#ifndef MEINEKRAFT_MESHMANAGER_HPP
#define MEINEKRAFT_MESHMANAGER_HPP

#include "primitives.h"

#include <iostream>
#include <vector>

struct MeshInformation {
    Mesh mesh;
    std::string loaded_from_filepath;
};

struct MeshManager {
  static std::pair<ID, std::vector<std::pair<Texture::Type, std::string>>>
  load_mesh(const std::string& directory, const std::string& file);
  
  static Mesh mesh_from_id(ID id);

  static ID mesh_id_from_primitive(MeshPrimitive primitive);
};

#endif // MEINEKRAFT_MESHMANAGER_HPP
