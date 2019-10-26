#include "meshmanager.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/pbrmaterial.h>
#include "../util/filesystem.hpp"
#include "../util/logging.hpp"

#include <numeric> // std::iota
#include <cassert> // assert

#define VERBOSE_LEVEL_0
// #define VERBOSE_LEVEL_1
// #define VERBOSE_LEVEL_2

static std::vector<Mesh> loaded_meshes{Cube(), Cube(true), Sphere()};

// NOTE: Assuming the metallic-roughness material model of models loaded with GLTF.
// NOTE: Loads root node of whatever model format the file is
std::pair<ID, std::vector<std::pair<Texture::Type, std::string>>>
MeshManager::load_mesh(const std::string& directory, const std::string& file) {
    const std::string loaded_from_filepath = directory + file;

    Assimp::Importer importer;
    auto scene = importer.ReadFile(loaded_from_filepath.c_str(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
      Log::error(std::string(importer.GetErrorString()));
      return {};
    }

    #ifdef VERBOSE_LEVEL_0
    Log::info("Loading scene: " + file);
    Log::info("\t ... # meshes " + std::to_string(scene->mNumMeshes));
    Log::info("\t ... # materials " + std::to_string(scene->mNumMaterials));
    #endif

    std::vector<std::pair<Texture::Type, std::string>> texture_info;
    MeshInformation mesh_info;
    mesh_info.loaded_from_filepath = directory + file;
    if (scene->HasMeshes()) {
      auto mesh = scene->mMeshes[0];
      #ifdef VERBOSE_LEVEL_1
      Log::info("\t ... loading mesh with name: " + std::string(mesh->mName.data));
      #endif

      // Load all vertices
      for (size_t j = 0; j < mesh->mNumVertices; j++) {
        Vertex vertex;

        auto pos = mesh->mVertices[j];
        vertex.position = {pos.x, pos.y, pos.z};

        if (mesh->HasTextureCoords(0)) {
          auto tex_coord = mesh->mTextureCoords[0][j];
          vertex.tex_coord = {tex_coord.x, -tex_coord.y}; // glTF (& .obj) has a flipped texture coordinate system compared to OpenGL 
        }

        if (mesh->HasNormals()) {
          auto normal = mesh->mNormals[j];
          vertex.normal = {normal.x, normal.y, normal.z};
        }

        mesh_info.mesh.vertices.push_back(vertex);
      }

      // Load all indices from the faces
      for (size_t j = 0; j < mesh->mNumFaces; j++) {
        auto face = &mesh->mFaces[j];
        if (face->mNumIndices != 3) {
            Log::warn("Not 3 vertices per face in model.");
            return {};
        }
        for (size_t k = 0; k < 3; k++) {
          auto index = face->mIndices[k];
          mesh_info.mesh.indices.push_back(index);
        }
      } 
      loaded_meshes.push_back(mesh_info.mesh);

      if (scene->HasMaterials()) {
        auto material = scene->mMaterials[mesh->mMaterialIndex];

        #ifdef VERBOSE_LEVEL_1
        aiString material_name;
        material->Get(AI_MATKEY_NAME, material_name);
        Log::info("\t ... loading material: " + std::string(material_name.C_Str()));
        #endif

        aiString diffuse_filepath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_filepath) == AI_SUCCESS) {
          #ifdef VERBOSE_LEVEL_2
          Log::info("Diffuse texture name: " + std::string(directory.c_str()) + std::string(diffuse_filepath.data));
          #endif
          std::string texture_filepath(diffuse_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::Diffuse, texture_filepath });
        }

        aiString specular_filepath;
        if (material->GetTexture(aiTextureType_SPECULAR, 0, &specular_filepath) == AI_SUCCESS) {
          Log::info("Specular texture name: " + std::string(directory.c_str()) + std::string(specular_filepath.data));
        }

        aiString ambient_filepath;
        if (material->GetTexture(aiTextureType_AMBIENT, 0, &ambient_filepath) == AI_SUCCESS) {
          Log::info("Ambient occlusion texture name: " + std::string(directory.c_str()) + std::string(ambient_filepath.data));
        }

        aiString shininess_filepath;
        if (material->GetTexture(aiTextureType_SHININESS, 0, &shininess_filepath) == AI_SUCCESS) {
          Log::info("Shininess texture name: " + std::string(directory.c_str()) + std::string(shininess_filepath.data));
        }

        aiString emissive_filepath;
        if (material->GetTexture(aiTextureType_EMISSIVE, 0, &emissive_filepath) == AI_SUCCESS) {
          Log::info("Emissive texture name: " + std::string(directory.c_str()) + std::string(emissive_filepath.data));
          std::string texture_filepath(emissive_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::Emissive, texture_filepath });
          // TODO: Fetch emissive factor as well 
        }

        aiString displacement_filepath;
        if (material->GetTexture(aiTextureType_DISPLACEMENT, 0, &displacement_filepath) == AI_SUCCESS) {
          Log::info("Displacement texture name: " + std::string(directory.c_str()) + std::string(displacement_filepath.data));
        }

        aiString height_filepath;
        if (material->GetTexture(aiTextureType_HEIGHT, 0, &height_filepath) == AI_SUCCESS) {
          Log::info("Bumpmap texture name: " + std::string(directory.c_str()) + std::string(height_filepath.data));
        }

        // Lightmap is usually the ambient occlusion map ...
        aiString lightmap_filepath;
        if (material->GetTexture(aiTextureType_LIGHTMAP, 0, &lightmap_filepath) == AI_SUCCESS) {
          Log::info("Lightmap texture name: " + std::string(directory.c_str()) + std::string(lightmap_filepath.data));
          std::string texture_filepath(lightmap_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::AmbientOcclusion, texture_filepath });
        }

        aiString normals_filepath;
        if (material->GetTexture(aiTextureType_NORMALS, 0, &normals_filepath) == AI_SUCCESS) {
          #ifdef VERBOSE_LEVEL_2
          Log::info("Normals texture name: " + std::string(directory.c_str()) + std::string(normals_filepath.data));
          #endif
        }

        aiString reflection_filepath;
        if (material->GetTexture(aiTextureType_REFLECTION, 0, &reflection_filepath) == AI_SUCCESS) {
          Log::info("Reflection texture name: " + std::string(directory.c_str()) + std::string(reflection_filepath.data));
        }

        aiString opacity_filepath;
        if (material->GetTexture(aiTextureType_OPACITY, 0, &opacity_filepath) == AI_SUCCESS) {
          Log::info("Opacity texture name: " + std::string(directory.c_str()) + std::string(opacity_filepath.data));
        }

        // NOTE: Roughness metallic textures are not detected so here we are assuming this is the unknown texture of the material.
        aiString unknown_filepath;
        if (material->GetTexture(aiTextureType_UNKNOWN, 0, &unknown_filepath) == AI_SUCCESS) {
          #ifdef VERBOSE_LEVEL_2
          Log::info("PBR parameter texture name: " + std::string(directory.c_str()) + std::string(unknown_filepath.data));
          #endif
          std::string texture_filepath(normals_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::MetallicRoughness, texture_filepath });
        }
      }
    }

    return {loaded_meshes.size() - 1, texture_info};
}

// Loads all the meshes in a given model file
std::pair<std::vector<ID>, std::vector<std::vector<std::pair<Texture::Type, std::string>>>>
MeshManager::load_meshes(const std::string& directory, const std::string& file) {
  const std::string loaded_from_filepath = directory + file;

  Assimp::Importer importer;
  auto scene = importer.ReadFile(loaded_from_filepath.c_str(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

  if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
    Log::error(std::string(importer.GetErrorString()));
    return {};
  }

  if (!scene->HasMeshes()) {
    if (std::strlen(importer.GetErrorString()) != 0) {
      Log::error(std::string(importer.GetErrorString()));
    }
    Log::error("Tried to load a Scene without any meshes from " + loaded_from_filepath);
    return {};
  }

  #ifdef VERBOSE_LEVEL_0
  Log::info("Loading scene: " + file);
  Log::info("\t ... # meshes " + std::to_string(scene->mNumMeshes));
  Log::info("\t ... # materials " + std::to_string(scene->mNumMaterials));
  #endif
  
  std::vector<std::vector<std::pair<Texture::Type, std::string>>> texture_infos;
  if (scene->HasMeshes()) {
    // FIXME: Assumes the mesh is a single mesh and not a hierarchy
    for (size_t mesh_idx = 0; mesh_idx < scene->mNumMeshes; mesh_idx++) {
      MeshInformation mesh_info; 

      auto mesh = scene->mMeshes[mesh_idx];

      #ifdef VERBOSE_LEVEL_1
      Log::info("\t ... loading mesh with name: " + std::string(mesh->mName.data));
      #endif

      // Load all vertices
      mesh_info.mesh.vertices.reserve(mesh->mNumVertices);
      for (size_t j = 0; j < mesh->mNumVertices; j++) {
        Vertex vertex;

        const auto pos = mesh->mVertices[j];
        vertex.position = { pos.x, pos.y, pos.z };

        if (mesh->HasTextureCoords(0)) {
          const auto tex_coord = mesh->mTextureCoords[0][j];
          vertex.tex_coord = { tex_coord.x, -tex_coord.y }; // glTF (& .obj) has a flipped texture coordinate system compared to OpenGL 
        }

        if (mesh->HasNormals()) {
          const auto normal = mesh->mNormals[j];
          vertex.normal = { normal.x, normal.y, normal.z };
        }

        if (mesh->HasTangentsAndBitangents()) {
          const auto tangent = mesh->mTangents[j];
          vertex.tangent = { tangent.x, tangent.y, tangent.z };
        }

        mesh_info.mesh.vertices.push_back(vertex);
      }

      // Load all indices from the faces
      mesh_info.mesh.indices.reserve(size_t(mesh->mNumFaces) * 3);
      for (size_t j = 0; j < mesh->mNumFaces; j++) {
        auto face = &mesh->mFaces[j];
        if (face->mNumIndices != 3) {
          Log::warn("Not 3 vertices per face in model.");
          return {};
        }
        for (size_t k = 0; k < 3; k++) {
          mesh_info.mesh.indices.push_back(face->mIndices[k]);
        }
      }
      loaded_meshes.push_back(mesh_info.mesh);

      if (scene->HasMaterials()) {
        auto material = scene->mMaterials[mesh->mMaterialIndex];

        aiColor3D bcf;
        if (material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, bcf) == AI_SUCCESS) {
          const auto base_color_factor = Vec3f(bcf[0], bcf[1], bcf[2]);
          #ifdef VERBOSE_LEVEL_2
          Log::info("\t ... base color factor (R, G, B): (" + std::to_string(bcf[0]) + ", " + std::to_string(bcf[1]) + ", " + std::to_string(bcf[2]) + ")");
          #endif
        }

        #ifdef VERBOSE_LEVEL_1
        aiString material_name;
        if (material->Get(AI_MATKEY_NAME, material_name) == AI_SUCCESS) {
          Log::info("\t ... loading material: " + std::string(material_name.C_Str()));
        } else {
          Log::info("\t ... loading material of unknown name");
        }
        #endif

        std::vector<std::pair<Texture::Type, std::string>> texture_info;

        aiString diffuse_filepath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_filepath) == AI_SUCCESS) {
          #ifdef VERBOSE_LEVEL_2
          Log::info("Diffuse texture name: " + std::string(directory.c_str()) + std::string(diffuse_filepath.data));
          #endif
          std::string texture_filepath(diffuse_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::Diffuse, texture_filepath });
        }

        aiString specular_filepath;
        if (material->GetTexture(aiTextureType_SPECULAR, 0, &specular_filepath) == AI_SUCCESS) {
          Log::info("Specular texture name: " + std::string(directory.c_str()) + std::string(specular_filepath.data));
        }

        aiString ambient_filepath;
        if (material->GetTexture(aiTextureType_AMBIENT, 0, &ambient_filepath) == AI_SUCCESS) {
          Log::info("Ambient occlusion texture name: " + std::string(directory.c_str()) + std::string(ambient_filepath.data));
        }

        aiString shininess_filepath;
        if (material->GetTexture(aiTextureType_SHININESS, 0, &shininess_filepath) == AI_SUCCESS) {
          Log::info("Shininess texture name: " + std::string(directory.c_str()) + std::string(shininess_filepath.data));
        }

        aiString emissive_filepath;
        if (material->GetTexture(aiTextureType_EMISSIVE, 0, &emissive_filepath) == AI_SUCCESS) {
          Log::info("Emissive texture name: " + std::string(directory.c_str()) + std::string(emissive_filepath.data));
          std::string texture_filepath(emissive_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::Emissive, texture_filepath });
          // TODO: Fetch emissive factor as well 
        }

        // NOTE: A.k.a bump map ...
        aiString displacement_filepath;
        if (material->GetTexture(aiTextureType_DISPLACEMENT, 0, &displacement_filepath) == AI_SUCCESS) {
          Log::info("Displacement texture name: " + std::string(directory.c_str()) + std::string(displacement_filepath.data));
        }

        aiString height_filepath;
        if (material->GetTexture(aiTextureType_HEIGHT, 0, &height_filepath) == AI_SUCCESS) {
          Log::info("Bumpmap texture name: " + std::string(directory.c_str()) + std::string(height_filepath.data));
        }

        // NOTE: Lightmap is usually the ambient occlusion map ...
        // NOTE: .. and some times the (occlusion, roughness, metallic) parameter texture for glTF models ...
        aiString lightmap_filepath;
        if (material->GetTexture(aiTextureType_LIGHTMAP, 0, &lightmap_filepath) == AI_SUCCESS) {
          Log::info("Lightmap texture name: " + std::string(directory.c_str()) + std::string(lightmap_filepath.data));
          std::string texture_filepath(lightmap_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::MetallicRoughness, texture_filepath });
        }

        aiString normals_filepath;
        if (material->GetTexture(aiTextureType_NORMALS, 0, &normals_filepath) == AI_SUCCESS) {
          #ifdef VERBOSE_LEVEL_2
          Log::info("Normals texture name: " + std::string(directory.c_str()) + std::string(normals_filepath.data));
          #endif
          std::string texture_filepath(normals_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::TangentNormal, texture_filepath });
        }

        aiString reflection_filepath;
        if (material->GetTexture(aiTextureType_REFLECTION, 0, &reflection_filepath) == AI_SUCCESS) {
          Log::info("Reflection texture name: " + std::string(directory.c_str()) + std::string(reflection_filepath.data));
        }

        aiString opacity_filepath;
        if (material->GetTexture(aiTextureType_OPACITY, 0, &opacity_filepath) == AI_SUCCESS) {
          Log::info("Opacity texture name: " + std::string(directory.c_str()) + std::string(opacity_filepath.data));
        }

        aiString diffuse_roughness_filepath;
        if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &diffuse_roughness_filepath) == AI_SUCCESS) {
          Log::info("Diffuse roughness texture name: " + std::string(directory.c_str()) + std::string(diffuse_roughness_filepath.data));
        }

        aiString metallic_filepath;
        if (material->GetTexture(aiTextureType_METALNESS, 0, &metallic_filepath) == AI_SUCCESS) {
          Log::info("Metallic texture name: " + std::string(directory.c_str()) + std::string(metallic_filepath.data));
        }

        // NOTE: Roughness metallic textures are not detected so here we are assuming this is the unknown texture of the material.
        aiString unknown_filepath;
        if (material->GetTexture(aiTextureType_UNKNOWN, 0, &unknown_filepath) == AI_SUCCESS) {
          #ifdef VERBOSE_LEVEL_2
          Log::info("PBR parameter texture name: " + std::string(directory.c_str()) + std::string(unknown_filepath.data));
          #endif
          std::string texture_filepath(unknown_filepath.data);
          texture_filepath.insert(0, directory);
          texture_info.push_back({ Texture::Type::MetallicRoughness, texture_filepath });
        }
        texture_infos.push_back(texture_info);
      }
    }
  }

  std::vector<ID> mesh_ids(scene->mNumMeshes);
  std::iota(mesh_ids.begin(), mesh_ids.end(), loaded_meshes.size() - mesh_ids.size());
  assert(mesh_ids.size() == texture_infos.size());
  return { mesh_ids, texture_infos };
}

Mesh MeshManager::mesh_from_id(ID id) {
  if (id < loaded_meshes.size()) {
    return loaded_meshes[id];
  } else {
    Log::error("Non existent mesh id provided.");
  }
  return {};
}
