#include "AssimpSceneImporter.h"

#include <KrisLogger/Logger.h>
#if RETRO_WITH_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <utility>
#include <vector>

#ifdef __ANDROID__
#include "../native/AndroidBridge.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

namespace RetroRenderer {
namespace {
glm::mat4 AssimpToGlmMatrix(const aiMatrix4x4& mat) {
    return glm::mat4(mat[0][0],
                     mat[1][0],
                     mat[2][0],
                     mat[3][0],
                     mat[0][1],
                     mat[1][1],
                     mat[2][1],
                     mat[3][1],
                     mat[0][2],
                     mat[1][2],
                     mat[2][2],
                     mat[3][2],
                     mat[0][3],
                     mat[1][3],
                     mat[2][3],
                     mat[3][3]);
}

ImportedMaterial ConvertAssimpMaterial(const aiMaterial* material) {
    ImportedMaterial importedMaterial{};
    if (!material) {
        return importedMaterial;
    }

    aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
    importedMaterial.diffuseColor = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);

    aiColor3D specularColor(0.0f, 0.0f, 0.0f);
    material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
    importedMaterial.specularColor = glm::vec3(specularColor.r, specularColor.g, specularColor.b);

    return importedMaterial;
}

ImportedMesh ConvertAssimpMesh(const aiMesh* mesh, const aiScene* scene) {
    ImportedMesh importedMesh{};
    if (!mesh) {
        return importedMesh;
    }

    const aiMaterial* material = nullptr;
    if (scene && mesh->mMaterialIndex < scene->mNumMaterials) {
        material = scene->mMaterials[mesh->mMaterialIndex];
        importedMesh.materialIndex = static_cast<int>(mesh->mMaterialIndex);
    }

    importedMesh.vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};
        vertex.position.w = 1.0f;
        vertex.color = glm::vec3(1.0f);

        if (mesh->HasPositions()) {
            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;
        }
        if (mesh->HasNormals()) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
        }
        if (material) {
            aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
            vertex.color = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
        }

        importedMesh.vertices.push_back(vertex);
    }

    importedMesh.indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices != 3) {
            LOGW("Skipping face that doesn't have exactly 3 indices.");
            continue;
        }
        importedMesh.indices.push_back(face.mIndices[0]);
        importedMesh.indices.push_back(face.mIndices[1]);
        importedMesh.indices.push_back(face.mIndices[2]);
    }

    return importedMesh;
}

int ConvertAssimpNode(const aiNode* node, const aiScene* scene, ImportedSceneData& outScene) {
    if (!node || !scene) {
        return -1;
    }

    ImportedNode importedNode{};
    importedNode.name = node->mName.C_Str();
    importedNode.localTransform = AssimpToGlmMatrix(node->mTransformation);
    importedNode.meshIndices.reserve(node->mNumMeshes);
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        const int meshIndex = static_cast<int>(node->mMeshes[i]);
        if (meshIndex < 0 || meshIndex >= static_cast<int>(scene->mNumMeshes)) {
            LOGW("Skipping invalid mesh index %d in node %s", meshIndex, node->mName.C_Str());
            continue;
        }
        importedNode.meshIndices.push_back(meshIndex);
    }

    const int currentNodeIndex = static_cast<int>(outScene.nodes.size());
    outScene.nodes.emplace_back(std::move(importedNode));

    outScene.nodes[currentNodeIndex].childNodeIndices.reserve(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        const int childNodeIndex = ConvertAssimpNode(node->mChildren[i], scene, outScene);
        if (childNodeIndex >= 0) {
            outScene.nodes[currentNodeIndex].childNodeIndices.push_back(childNodeIndex);
        }
    }

    return currentNodeIndex;
}
} // namespace

bool AssimpSceneImporter::LoadFromMemory(const uint8_t* data, size_t size, ImportedSceneData& outSceneData) {
    if (!data || size == 0) {
        LOGE("assimp: Tried to load scene from empty memory buffer");
        return false;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(data, size, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOGE("assimp: Failed to load scene from memory: %s", importer.GetErrorString());
        return false;
    }
    return BuildImportedSceneData(scene, outSceneData);
}

bool AssimpSceneImporter::LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) {
    Assimp::Importer importer;
    const aiScene* scene = nullptr;
#ifdef __ANDROID__
    AAsset* asset = AAssetManager_open(g_assetManager, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("assimp: Failed to open asset '%s'", path.c_str());
        return false;
    }

    const size_t fileSize = AAsset_getLength(asset);
    std::vector<uint8_t> buffer(fileSize);
    AAsset_read(asset, buffer.data(), fileSize);
    AAsset_close(asset);

    scene = importer.ReadFileFromMemory(buffer.data(), fileSize, aiProcess_Triangulate | aiProcess_FlipUVs);
#else
    scene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
#endif
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOGE("assimp: Failed to load scene from file '%s': %s", path.c_str(), importer.GetErrorString());
        return false;
    }
    return BuildImportedSceneData(scene, outSceneData);
}

bool AssimpSceneImporter::BuildImportedSceneData(const aiScene* scene, ImportedSceneData& outSceneData) const {
    if (!scene || !scene->mRootNode) {
        return false;
    }

    outSceneData = ImportedSceneData{};
    outSceneData.materials.reserve(scene->mNumMaterials);
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        outSceneData.materials.push_back(ConvertAssimpMaterial(scene->mMaterials[i]));
    }

    outSceneData.meshes.reserve(scene->mNumMeshes);
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        outSceneData.meshes.push_back(ConvertAssimpMesh(scene->mMeshes[i], scene));
    }

    outSceneData.rootNodeIndex = ConvertAssimpNode(scene->mRootNode, scene, outSceneData);
    return outSceneData.rootNodeIndex >= 0;
}
} // namespace RetroRenderer
#else

namespace RetroRenderer {
bool AssimpSceneImporter::LoadFromMemory(const uint8_t* data, size_t size, ImportedSceneData& outSceneData) {
    (void)data;
    (void)size;
    (void)outSceneData;
    LOGE("Assimp importer requested, but RETRO_WITH_ASSIMP=0");
    return false;
}

bool AssimpSceneImporter::LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) {
    (void)path;
    (void)outSceneData;
    LOGE("Assimp importer requested, but RETRO_WITH_ASSIMP=0");
    return false;
}

bool AssimpSceneImporter::BuildImportedSceneData(const aiScene* scene, ImportedSceneData& outSceneData) const {
    (void)scene;
    (void)outSceneData;
    return false;
}
} // namespace RetroRenderer
#endif
