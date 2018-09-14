#include "meshmanager.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <iostream>
#include "../util/filesystem.h"

static std::vector<Mesh> loaded_meshes{Cube()};

// Assuming the metallic-roughness material model of models loaded with GLTF.
std::pair<ID, std::vector<std::pair<Texture::Type, std::string>>>
MeshManager::load_mesh(const std::string& directory, const std::string& file) {
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
        SDL_Log("Number of materials: %i", scene->mNumMaterials);
        for (size_t i = 0; i < scene->mNumMaterials; i++) {
            auto material = scene->mMaterials[i];

            aiString material_name;
            material->Get(AI_MATKEY_NAME, material_name);
            SDL_Log("Material name: %s", material_name.C_Str());
          
            aiString diffuse_filepath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_filepath) == AI_SUCCESS) {
                SDL_Log("Diffuse texture name: %s%s", directory.c_str(), diffuse_filepath.data);
                std::string texture_filepath(diffuse_filepath.data);
                texture_filepath.insert(0, directory);
                texture_info.push_back({Texture::Type::Diffuse, texture_filepath});
            }
          
            aiString specular_filepath;
            if (material->GetTexture(aiTextureType_SPECULAR, 0, &specular_filepath) == AI_SUCCESS) {
              SDL_Log("Specular texture name: %s%s", directory.c_str(), specular_filepath.data);
            }

            aiString ambient_filepath;
            if (material->GetTexture(aiTextureType_AMBIENT, 0, &ambient_filepath) == AI_SUCCESS) {
              SDL_Log("Ambient texture name: %s%s", directory.c_str(), ambient_filepath.data);
            }
            
            aiString shininess_filepath;
            if (material->GetTexture(aiTextureType_SHININESS, 0, &shininess_filepath) == AI_SUCCESS) {
              SDL_Log("Shininess texture name: %s%s", directory.c_str(), shininess_filepath.data);
            }

            aiString emissive_filepath;
            if (material->GetTexture(aiTextureType_EMISSIVE, 0, &emissive_filepath) == AI_SUCCESS) {
              SDL_Log("Emissive texture name: %s%s", directory.c_str(), emissive_filepath.data);
              std::string texture_filepath(emissive_filepath.data);
              texture_filepath.insert(0, directory);
              texture_info.push_back({Texture::Type::Emissive, texture_filepath});
            }

            aiString displacement_filepath;
            if (material->GetTexture(aiTextureType_DISPLACEMENT, 0, &displacement_filepath) == AI_SUCCESS) {
              SDL_Log("Displacement texture name: %s%s", directory.c_str(), displacement_filepath.data);
            }

            aiString height_filepath;
            if (material->GetTexture(aiTextureType_HEIGHT, 0, &height_filepath) == AI_SUCCESS) {
              SDL_Log("Bumpmap texture name: %s%s", directory.c_str(), height_filepath.data);
            }

            // Lightmap is usually the ambient occlusion map ...
            aiString lightmap_filepath;
            if (material->GetTexture(aiTextureType_LIGHTMAP, 0, &lightmap_filepath) == AI_SUCCESS) {
              SDL_Log("Lightmap texture name: %s%s", directory.c_str(), lightmap_filepath.data);
              std::string texture_filepath(lightmap_filepath.data);
              texture_filepath.insert(0, directory);
              texture_info.push_back({Texture::Type::AmbientOcclusion, texture_filepath});
            }
             
            aiString normals_filepath;
            if (material->GetTexture(aiTextureType_NORMALS, 0, &normals_filepath) == AI_SUCCESS) {
              SDL_Log("Normals texture name: %s%s", directory.c_str(), normals_filepath.data);
              std::string texture_filepath(normals_filepath.data);
              texture_filepath.insert(0, directory);
              texture_info.push_back({Texture::Type::Normal, texture_filepath});
            }

            aiString reflection_filepath;
            if (material->GetTexture(aiTextureType_REFLECTION, 0, &reflection_filepath) == AI_SUCCESS) {
              SDL_Log("Reflection texture name: %s%s", directory.c_str(), reflection_filepath.data);
            }

            aiString opacity_filepath;
            if (material->GetTexture(aiTextureType_OPACITY, 0, &opacity_filepath) == AI_SUCCESS) {
              SDL_Log("Opacity texture name: %s%s", directory.c_str(), opacity_filepath.data);
            }
          
            // NOTE: Roughness metallic textures are not detected so here we are assuming this is the unknown texture of the material.
            aiString unknown_filepath;
            if (material->GetTexture(aiTextureType_UNKNOWN, 0, &unknown_filepath) == AI_SUCCESS) {
              SDL_Log("Unknown texture name: %s%s", directory.c_str(), unknown_filepath.data);
              std::string texture_filepath(normals_filepath.data);
              texture_filepath.insert(0, directory);
              texture_info.push_back({Texture::Type::MetallicRoughness, texture_filepath});
            }
        }
    }

    if (scene->HasMeshes()) {
        // FIXME: Assumes the mesh is a single mesh and not a hierarchy
        SDL_Log("Scene has %u meshes", scene->mNumMeshes);
        for (size_t i = 0; i < scene->mNumMeshes; i++) {
            auto mesh = scene->mMeshes[i];

            for (size_t j = 0; j < mesh->mNumVertices; j++) {
                Vertex<float> vertex;

                auto pos = mesh->mVertices[j];
                vertex.position = {pos.x, pos.y, pos.z};

                if (mesh->HasTextureCoords(0)) {
                    auto texCoord = mesh->mTextureCoords[0][j];
                    vertex.texCoord = {texCoord.x, texCoord.y};
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
    // FIXME: Mesh id is worthless since it does not change or anything ...
    loaded_meshes.push_back(mesh_info.mesh);
    return {loaded_meshes.size() - 1, texture_info};
}

Mesh MeshManager::mesh_from_id(ID id) {
  if (id < loaded_meshes.size()) {
    return loaded_meshes[id];
  } else {
    std::cerr << "Error: Non existent mesh id provided." << std::endl;
  }
}

ID MeshManager::mesh_id_from_primitive(MeshPrimitive primitive) {
  // FIXME: Needs a proper solution ... like the rest of the mesh handling ...
  if (primitive == MeshPrimitive::Cube) {
    return 0;
  } else {
    std::cerr << "Loaded unknown primitive" << std::endl;
  }
}
