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
public:
    MeshManager();

    /// Return the mesh_id if flag is true
    std::pair<uint64_t, bool> is_mesh_loaded(std::string filepath, std::string directory);

    /**
     * Loads a model file and its textures, blocking.
     * @param filepath Filepath to the model file to be loaded
     * @return Mesh ID that identifies the loaded Mesh and a list of Textures (type, filepath).
     */
    std::pair<uint64_t, std::vector<std::pair<Texture::Type, std::string>>>
    load_mesh_from_file(std::string filepath, std::string directory);

    Mesh mesh_from_id(uint64_t mesh_id);

    /// Number of meshes loaded
    uint64_t meshes_loaded;

    uint64_t mesh_id_from_primitive(MeshPrimitive primitive);
};

#endif //MEINEKRAFT_MESHMANAGER_HPP
