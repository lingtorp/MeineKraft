#include "meshmanager.h"
#include <SDL2/SDL_log.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <iostream>

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
MeshManager::load_mesh(std::string directory, std::string file) {
    MeshInformation mesh_info;
    mesh_info.loaded_from_filepath = directory + file;

    Assimp::Importer importer;
    auto scene = importer.ReadFile(mesh_info.loaded_from_filepath.c_str(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        SDL_Log("Error: %s", importer.GetErrorString());
        return {0, {}};
    }

    std::vector<std::pair<Texture::Type, std::string>> texture_info;
    if (scene->HasMaterials()) {
        for (size_t i = 0; i < scene->mNumMaterials; i++) {
            auto material = scene->mMaterials[i];

            aiString material_name;
            material->Get(AI_MATKEY_NAME, material_name);
            SDL_Log("Material name: %s", material_name.C_Str());

            aiString tex_filepath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &tex_filepath) == AI_SUCCESS) {
                SDL_Log("%s%s, %i", directory.c_str(), tex_filepath.data, material->GetTextureCount(aiTextureType_DIFFUSE));
                std::string texture_filepath(tex_filepath.data);
                texture_filepath.insert(0, directory);
                texture_info.push_back({Texture::Type::Diffuse, texture_filepath});
            }
            // TODO: Load other texture types
        }
    }

    if (scene->HasMeshes()) {
        // FIXME: Assumes the mesh is a single mesh and not a hierarchy
        for (size_t i = 0; i < scene->mNumMeshes; i++) {
            auto mesh = scene->mMeshes[i];

            for (size_t j = 0; j < mesh->mNumVertices; j++) {
                Vertex<float> vertex;

                auto pos = mesh->mVertices[j];
                vertex.position = {pos.x, pos.y, pos.z};

                if (mesh->HasTextureCoords(0)) {
                    auto texCoord = mesh->mTextureCoords[0][j];
                    // FIXME: Assumes the model file is a .obj file with an inverted y-axis
                    vertex.texCoord = {texCoord.x, -texCoord.y};
                }

                if (mesh->HasNormals()) {
                    auto normal = mesh->mNormals[j];
                    vertex.normal = {normal.x, normal.y, normal.z};
                }

                mesh_info.mesh.vertices.push_back(vertex);
            }

            for (size_t j = 0; j < mesh->mNumFaces; j++) {
                auto face = &mesh->mFaces[j];
                if (face->mNumIndices != 3) {
                    SDL_Log("Not 3 vertices per face.");
                    return {0, {}};
                }
                for (size_t k = 0; k < 3; k++) {
                    auto index = face->mIndices[k];
                    mesh_info.mesh.indices.push_back(index);
                }
            }
        }
    }
    
    meshes_loaded++; // FIXME: Always returns the same mesh id; detroys the point of instanced rendering
    meshes[meshes_loaded] = mesh_info;
    return {meshes_loaded, texture_info};
}

Mesh MeshManager::mesh_from_id(uint64_t mesh_id) {
    return meshes[mesh_id].mesh;
}

uint64_t MeshManager::mesh_id_from_primitive(MeshPrimitive primitive) {
    switch (primitive) {
        case MeshPrimitive::Cube:
            return 0;
    }
}
