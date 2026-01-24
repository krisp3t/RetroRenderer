#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#ifdef __ANDROID__
#include "../native/AndroidBridge.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

#include "Scene.h"
#include <KrisLogger/Logger.h>
#include "../Engine.h"

#include <filesystem>
#include <format>

#include "Vertex.h"

namespace RetroRenderer {
bool Scene::Load(const uint8_t* data, const size_t size) {
    Assimp::Importer importer;
    const aiScene* scene = nullptr;
    scene = importer.ReadFileFromMemory(data, size, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOGE("assimp: Failed to load scene: %s", importer.GetErrorString());
        return false;
    }
    return ProcessScene(scene);
}

bool Scene::Load(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = nullptr;
#ifdef __ANDROID__
    AAsset* asset = AAssetManager_open(g_assetManager, path.c_str(), AASSET_MODE_BUFFER);
    if (asset) {
        size_t fileSize = AAsset_getLength(asset);
        std::vector<uint8_t> buffer(fileSize);
        AAsset_read(asset, buffer.data(), fileSize);
        AAsset_close(asset);

        scene = importer.ReadFileFromMemory(buffer.data(), fileSize, aiProcess_Triangulate | aiProcess_FlipUVs);
    }
#else
    scene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
#endif
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOGE("assimp: Failed to load scene: %s", importer.GetErrorString());
        return false;
    }
    return ProcessScene(scene);
}

bool Scene::ProcessScene(const aiScene* scene) {
    if (ProcessNode(scene->mRootNode, scene)) {
        // LOGI("Successfully processed scene: %s (%d meshes)", scene->mRootNode->mName.C_Str(), m_Meshes.size());
        LOGI("Successfully processed scene: %s", scene->mRootNode->mName.C_Str());
    }
    return true;
}

/**
 * Recursively process each node in the scene. One node will map to one model, which can have multiple meshes.
 * @param node
 * @param scene
 * @return true if successful
 */
bool Scene::ProcessNode(aiNode* node, const aiScene* scene, int parentIndex) {
    if (!node) {
        LOGE("assimp: Node is null");
        return false;
    }
    LOGD("Processing node: %s (%d meshes), parent index: %d", node->mName.C_Str(), node->mNumMeshes, parentIndex);

    // Create a model for this aiNode
    Model newModel{};
    newModel.Init(this, node->mName.C_Str(), node->mTransformation);
    // Process all meshes for this model
    for (size_t i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (mesh->mNumVertices > 0 && mesh->mNumFaces > 0) {
            ProcessMesh(mesh, scene, newModel.m_Meshes, node->mName);
            newModel.m_Meshes.back().Init();
        }
    }
    int currentNodeIndex = m_Models.size();
    if (parentIndex != -1) {
        newModel.SetParent(parentIndex);
        m_Models[parentIndex].m_Children.push_back(currentNodeIndex);
    }
    m_Models.emplace_back(std::move(newModel));

    // Recursively process children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene, currentNodeIndex);
    }
    return true;
}

/**
 * Process a root node in the scene
 * @param node
 * @param scene
 * @return true if successful
 */
bool Scene::ProcessNode(aiNode* node, const aiScene* scene) {
    return ProcessNode(node, scene, -1);
}

void Scene::ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<Mesh>& meshes, const aiString& modelName) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    aiMaterial* material;
    if (mesh->mMaterialIndex >= 0) {
        material = scene->mMaterials[mesh->mMaterialIndex];
    }

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};
        if (mesh->HasPositions()) {
            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;
            vertex.position.w = 1.0f;
        }
        if (mesh->HasNormals()) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
            // TODO: w?
        }
        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec{};
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoords = vec;
        } else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }
        if (material) {
            aiColor3D diffuseColor(0.f, 0.f, 0.f);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
            vertex.color = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
        } else {
            vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
        }
        vertices.push_back(vertex);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices != 3) {
            LOGW("Skipping face that doesn't have exactly 3 indices.");
            continue;
        }
        for (unsigned int j = 0; j < 3; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        LOGI("Processing material %s", material->GetName().C_Str());
        aiColor3D diffuseColor(0.f, 0.f, 0.f);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
        aiColor3D specularColor(0.f, 0.f, 0.f);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
        // TODO: process material
    }

    // TODO: do not hardcode like this
    /*
    Texture texture;
    std::string texPath = std::format("{}.png", modelName.C_Str());
    std::filesystem::path fullPath = std::filesystem::absolute(texPath);
    if (texture.LoadFromFile(fullPath.string().c_str()))
    {
        textures.emplace_back(std::move(texture));
    }
    */
    meshes.emplace_back(std::move(vertices), std::move(indices), std::move(textures));
}

std::vector<int>& Scene::GetVisibleModels() {
    return m_VisibleModels;
}

// TODO: current implementation uses the model origin only (no bounds)
// TODO: mesh AABB?
void Scene::FrustumCull(const Camera& camera) {
    m_VisibleModels.clear();
    m_VisibleModels.reserve(m_Models.size());

    const auto& cfg = Engine::Get().GetConfig()->cull;
    if (!cfg.frustumCull) {
        for (size_t i = 0; i < m_Models.size(); i++) {
            m_VisibleModels.push_back(i);
        }
        return;
    }

    const glm::mat4 vp = camera.m_ProjMat * camera.m_ViewMat;
    for (size_t i = 0; i < m_Models.size(); i++) {
        const glm::mat4 modelMat = m_Models[i].GetWorldTransform();
        const glm::vec4 clipPos = vp * modelMat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (clipPos.w == 0.0f) {
            continue;
        }
        const float w = clipPos.w;
        if (clipPos.x < -w || clipPos.x > w ||
            clipPos.y < -w || clipPos.y > w ||
            clipPos.z < -w || clipPos.z > w) {
            continue;
        }
        m_VisibleModels.push_back(i);
    }
}

const glm::mat4& Scene::GetModelWorldTransform(int index) const {
    assert(index >= 0 && index < m_Models.size() && "Invalid model index");
    return m_Models[index].GetWorldTransform();
}

void Scene::MarkDirtyModel(int index) {
    m_Models[index].MarkDirty();
}
} // namespace RetroRenderer
