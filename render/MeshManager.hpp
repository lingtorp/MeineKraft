#ifndef MEINEKRAFT_MESHMANAGER_HPP
#define MEINEKRAFT_MESHMANAGER_HPP

#include <unordered_map>
#include "primitives.h"

struct MeshInformation {
    Mesh mesh;
    std::string loaded_from_filepath;
};

/**
 * Loads, stores, Meshes used by the Renderer
 */
class MeshManager {
private:
    std::unordered_map<uint64_t, MeshInformation> meshes;

    /// Loads a mesh from a file
    Mesh load_obj_mesh_from_file(std::string filepath, std::string directory);
public:
    MeshManager();

    /// Return the mesh_id if flag is true
    std::pair<uint64_t, bool> is_mesh_loaded(std::string filepath, std::string directory);

    /// Loads the mesh from the specified file (only .obj at this point)
    uint64_t load_mesh_from_file(std::string filepath, std::string directory);

    Mesh mesh_from_id(uint64_t mesh_id);

    /// Number of meshes loaded
    uint64_t meshes_loaded;

    uint64_t mesh_id_from_primitive(MeshPrimitive primitive);
};

#endif //MEINEKRAFT_MESHMANAGER_HPP
