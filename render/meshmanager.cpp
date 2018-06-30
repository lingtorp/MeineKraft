#include "meshmanager.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <iostream>
#include "../util/filesystem.h"

std::pair<ID, std::vector<std::pair<Texture::Type, std::string>>>
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
                SDL_Log("Diffuse texture name: %s%s", directory.c_str(), tex_filepath.data);
                std::string texture_filepath(tex_filepath.data);
                texture_filepath.insert(0, directory);
                texture_info.push_back({Texture::Type::Diffuse, texture_filepath});
            }
          
            float shininess;
            material->Get(AI_MATKEY_SHININESS, shininess);
            SDL_Log("Material shininess: %f", shininess);
          
            if (material->GetTexture(aiTextureType_SPECULAR, 0, &tex_filepath) == AI_SUCCESS) {
              SDL_Log("Specular texture name: %s%s", directory.c_str(), tex_filepath.data);
              std::string texture_filepath(tex_filepath.data);
              texture_filepath.insert(0, directory);
              texture_info.push_back({Texture::Type::Specular, texture_filepath});
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
    loaded_meshes.push_back(mesh_info.mesh); // FIXME: Always returns the same mesh id; detroys the point of instanced rendering
    return {loaded_meshes.size() - 1, texture_info};
}