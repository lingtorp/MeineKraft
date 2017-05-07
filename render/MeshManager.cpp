#include "MeshManager.hpp"
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
MeshManager::load_mesh_from_file(std::string filepath,
                                 std::string directory) {
    Assimp::Importer importer;
    auto scene = importer.ReadFile(filepath.c_str(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (scene == nullptr) {
        SDL_Log("Error: %s", importer.GetErrorString());
        return {0, {}};
    }

    std::vector<std::pair<Texture::Type, std::string>> texture_info;
    MeshInformation mesh_info;
    mesh_info.loaded_from_filepath = directory + filepath;

    if (scene->HasMaterials()) {
        for (size_t i = 0; i < scene->mNumMaterials; i++) {
            auto material = scene->mMaterials[i];

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

    if (scene->HasMeshes()) {
        for (size_t i = 0; i < scene->mNumMeshes; i++) {
            auto mesh = scene->mMeshes[i];

            for (size_t i = 0; i < mesh->mNumVertices; i++) {
                Vertex<float> vertex;

                auto pos = mesh->mVertices[i];
                vertex.position = {pos.x, pos.y, pos.z};

                if (mesh->HasTextureCoords(0)) {
                    auto texCoord = mesh->mTextureCoords[0][i];
                    // FIXME: Assumes the model file is a .obj file with an inverted y-axis
                    vertex.texCoord = {texCoord.x, -texCoord.y};
                }

                if (mesh->HasNormals()) {
                    auto normal = mesh->mNormals[i];
                    vertex.normal = {normal.x, normal.y, normal.z};
                }

                mesh_info.mesh.vertices.push_back(vertex);
            }

            for (size_t i = 0; i < mesh->mNumFaces; i++) {
                auto face = &mesh->mFaces[i];
                if (face->mNumIndices != 3) {
                    SDL_Log("Not 3 vertices per face.");
                    return {0, {}};
                }
                for (size_t j = 0; j < 3; j++) {
                    auto index = face->mIndices[j];
                    mesh_info.mesh.indices.push_back(index);
                }
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

uint64_t MeshManager::mesh_id_from_primitive(MeshPrimitive primitive) {
    switch (primitive) {
        case MeshPrimitive::Cube:
            return 0;
    }
}
