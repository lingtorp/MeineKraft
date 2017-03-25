#include "MeshManager.hpp"
#include <SDL2/SDL_log.h>
#include "../include/tinyobjloader/tiny_obj_loader.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

MeshManager::MeshManager(): meshes{}, meshes_loaded(0) {
    meshes[meshes_loaded++] = {Cube(), ""};
}

std::pair<uint64_t, bool> MeshManager::is_mesh_loaded(std::string filepath, std::string directory) {
    for (auto &entry : meshes) {
        if (entry.second.loaded_from_filepath == filepath + directory) {
            return {entry.first, true};
        }
    }
    return {0, false};
}

std::pair<uint64_t, std::vector<std::pair<Texture::Type, std::string>>>
MeshManager::load_mesh_from_file(std::string filepath,
                                 std::string directory) {
    Assimp::Importer importer;
    auto result = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (result == nullptr) {
        SDL_Log("Error: %s", importer.GetErrorString());
        return {0, {}};
    }

    std::vector<std::pair<Texture::Type, std::string>> texture_info;
    MeshInformation mesh_info;
    mesh_info.loaded_from_filepath = directory + filepath;

    if (result->HasMaterials()) {
        for (size_t i = 0; i < result->mNumMaterials; i++) {
            auto material = result->mMaterials[i];
            aiString material_name;
            material->Get(AI_MATKEY_NAME, material_name);
            SDL_Log("%s", material_name.C_Str());

            aiString tex_filepath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &tex_filepath) == AI_SUCCESS) {
                SDL_Log("%s%s, %i", directory.c_str(), tex_filepath.data, material->GetTextureCount(aiTextureType_DIFFUSE));
                std::string texture_filepath(tex_filepath.data);
                texture_filepath.insert(0, directory);
                // TODO: Cache textures in TextureManager
                texture_info.push_back({Texture::Type::Diffuse, texture_filepath});
            }
        }
    }

    if (result->HasMeshes()) {
        for (size_t i = 0; i < result->mNumMeshes; i++) {
            auto mesh = result->mMeshes[i];
            for (size_t i = 0; i < mesh->mNumVertices; i++) {
                auto position = mesh->mVertices[i];
                Vertex<float> vertex;
                vertex.position.x = position.x;
                vertex.position.y = position.y;
                vertex.position.z = position.z;
                mesh_info.mesh.vertices.push_back(vertex);
                aiVector3D *tex_coords = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i] : nullptr;
                if (tex_coords) {
                    vertex.texCoord.x = tex_coords->x;
                    vertex.texCoord.y = tex_coords->y;
                }
                if (mesh->HasNormals()) {
                    auto normal = mesh->mNormals[i];
                    vertex.normal.x = normal.x;
                    vertex.normal.y = normal.y;
                    vertex.normal.z = normal.z;
                }
            }

            for (size_t i = 0; i < mesh->mNumFaces; i++) {
                auto face = &mesh->mFaces[i];
                if (face->mNumIndices != 3) {
                    SDL_Log("Not 3 vertices per face.");
                    return {0, {}};
                }
                mesh_info.mesh.indices.push_back(face->mIndices[0]);
                mesh_info.mesh.indices.push_back(face->mIndices[1]);
                mesh_info.mesh.indices.push_back(face->mIndices[2]);
            }
        }
    }

    meshes_loaded++;
    meshes[meshes_loaded] = mesh_info;
    return {meshes_loaded, texture_info};
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

    SDL_Log("Number of vertices: %lu for model %s", mesh.vertices.size(), filepath.c_str());
    return mesh;
}

uint64_t MeshManager::mesh_id_from_primitive(MeshPrimitive primitive) {
    switch (primitive) {
        case MeshPrimitive::Cube:
            return 0;
    }
}
