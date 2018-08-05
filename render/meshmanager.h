#ifndef MEINEKRAFT_MESHMANAGER_HPP
#define MEINEKRAFT_MESHMANAGER_HPP

#include "primitives.h"
#include <iostream>

struct MeshInformation {
    Mesh mesh;
    std::string loaded_from_filepath;
};

struct MeshManager {
  std::vector<Mesh> loaded_meshes{};
  
  std::pair<ID, std::vector<std::pair<Texture::Type, std::string>>>
  load_mesh(std::string directory, std::string file);
  
  Mesh mesh_from_id(ID id) {
    switch (id) {
      case 0:
        return Cube();
      default:
        exit(1);
        std::cerr << "Error in meshmanager.hpp" << std::endl;  
    }
    // FIXME: There is no mechanism behind this at all
    return loaded_meshes[id];
  }
};

#endif //MEINEKRAFT_MESHMANAGER_HPP
