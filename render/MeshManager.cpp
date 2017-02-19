#include "MeshManager.hpp"
#include <SDL2/SDL_log.h>
#include "../include/tinyobjloader/tiny_obj_loader.h"

MeshManager::MeshManager(): meshes{} {}

std::pair<uint64_t, bool> MeshManager::is_mesh_loaded(std::string filepath, std::string directory) {
    for (auto &entry : meshes) {
        if (entry.second.loaded_from_filepath == filepath + directory) {
            return {entry.first, true};
        }
    }
    return {0, false};
}

uint64_t MeshManager::load_mesh_from_file(std::string filepath, std::string directory) {
    auto mesh = load_obj_mesh_from_file(filepath, directory);
    MeshInformation mesh_info{mesh, filepath + directory};
    meshes[++meshes_loaded] = mesh_info;
    return meshes_loaded;
}

Mesh MeshManager::mesh_from_id(uint64_t mesh_id) {
    return meshes[mesh_id].mesh;
}

Mesh MeshManager::load_obj_mesh_from_file(std::string filepath, std::string directory) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    auto success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath.c_str(), directory.c_str(), true);
    if (!success) { SDL_Log("Failed loading mesh %s: %s", filepath.c_str(), err.c_str()); return Mesh(); }
    if (err.size() > 0) { SDL_Log("%s", err.c_str()); }

    std::unordered_map<Vertex<float>, size_t> unique_vertices{};
    Mesh mesh{};
    for (const auto &shape : shapes) { // Shapes
        for (const auto &idx : shape.mesh.indices) { // Faces
            Vertex<float> vertex{};
            float vx = attrib.vertices[3 * idx.vertex_index + 0];
            float vy = attrib.vertices[3 * idx.vertex_index + 1];
            float vz = attrib.vertices[3 * idx.vertex_index + 2];
            vertex.position = {vx, vy, vz};

            float tx = attrib.texcoords[2 * idx.texcoord_index + 0];
            float ty = attrib.texcoords[2 * idx.texcoord_index + 1];
            vertex.texCoord = {tx, 1.0f - ty}; // .obj format has flipped y-axis compared to OpenGL

            float nx = attrib.normals[3 * idx.normal_index + 0];
            float ny = attrib.normals[3 * idx.normal_index + 1];
            float nz = attrib.normals[3 * idx.normal_index + 2];
            vertex.normal = Vec3<float>{nx, ny, nz}.normalize();

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = mesh.vertices.size();
                mesh.indices.push_back(mesh.vertices.size());
                mesh.vertices.push_back(vertex);
            } else {
                mesh.indices.push_back(unique_vertices.at(vertex));
            }
        }
    }

    std::unordered_map<std::string, uint64_t> loaded_textures{};
    for (const auto &material : materials) {
        /// Color map, a.k.a diffuse map
        if (!loaded_textures[directory + material.diffuse_texname]) {
            Texture diffuse_texture{};
            diffuse_texture.load(material.diffuse_texname, directory);
            if (diffuse_texture.loaded_succesfully) {
                mesh.texture = diffuse_texture;
                loaded_textures[material.diffuse_texname] = diffuse_texture.gl_texture;
            }
        }
    }

    SDL_Log("Number of vertices: %lu for model %s", mesh.vertices.size(), filepath.c_str());
    return mesh;
}
