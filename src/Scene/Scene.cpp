#include "Scene.h"
#include "ISceneImporter.h"
#include "../Engine.h"
#include <KrisLogger/Logger.h>
#include <array>
#include <filesystem>
#include <limits>
#include <utility>

namespace RetroRenderer {
namespace {
struct FrustumPlane {
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
    float distance = 0.0f;
};

glm::vec4 GetMatrixRow(const glm::mat4& matrix, int row) {
    return glm::vec4(matrix[0][row], matrix[1][row], matrix[2][row], matrix[3][row]);
}

FrustumPlane MakeNormalizedPlane(const glm::vec4& coefficients) {
    const glm::vec3 normal(coefficients.x, coefficients.y, coefficients.z);
    const float length = glm::length(normal);
    if (length <= 1e-6f) {
        return {};
    }
    return {normal / length, coefficients.w / length};
}

std::array<FrustumPlane, 6> ExtractFrustumPlanes(const glm::mat4& viewProjection) {
    const glm::vec4 row0 = GetMatrixRow(viewProjection, 0);
    const glm::vec4 row1 = GetMatrixRow(viewProjection, 1);
    const glm::vec4 row2 = GetMatrixRow(viewProjection, 2);
    const glm::vec4 row3 = GetMatrixRow(viewProjection, 3);

    return {
        MakeNormalizedPlane(row3 + row0), // left
        MakeNormalizedPlane(row3 - row0), // right
        MakeNormalizedPlane(row3 + row1), // bottom
        MakeNormalizedPlane(row3 - row1), // top
        MakeNormalizedPlane(row3 + row2), // near
        MakeNormalizedPlane(row3 - row2), // far
    };
}

void TransformBounds(const glm::mat4& transform, const glm::vec3& localMin, const glm::vec3& localMax, glm::vec3& outMin, glm::vec3& outMax) {
    static constexpr std::array<glm::vec3, 8> kCorners = {
        glm::vec3{0.0f, 0.0f, 0.0f},
        glm::vec3{1.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 1.0f, 0.0f},
        glm::vec3{1.0f, 1.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, 1.0f},
        glm::vec3{1.0f, 0.0f, 1.0f},
        glm::vec3{0.0f, 1.0f, 1.0f},
        glm::vec3{1.0f, 1.0f, 1.0f},
    };

    const glm::vec3 extent = localMax - localMin;
    bool initialized = false;
    for (const glm::vec3& cornerWeights : kCorners) {
        const glm::vec3 localCorner = localMin + extent * cornerWeights;
        const glm::vec3 worldCorner = glm::vec3(transform * glm::vec4(localCorner, 1.0f));
        if (!initialized) {
            outMin = worldCorner;
            outMax = worldCorner;
            initialized = true;
            continue;
        }
        outMin = glm::min(outMin, worldCorner);
        outMax = glm::max(outMax, worldCorner);
    }
}

bool IsAabbOutsidePlane(const FrustumPlane& plane, const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
    glm::vec3 positiveVertex = boundsMin;
    if (plane.normal.x >= 0.0f) {
        positiveVertex.x = boundsMax.x;
    }
    if (plane.normal.y >= 0.0f) {
        positiveVertex.y = boundsMax.y;
    }
    if (plane.normal.z >= 0.0f) {
        positiveVertex.z = boundsMax.z;
    }

    return glm::dot(plane.normal, positiveVertex) + plane.distance < 0.0f;
}

bool IsModelVisibleInFrustum(const Model& model, const std::array<FrustumPlane, 6>& frustumPlanes) {
    glm::vec3 localMin{};
    glm::vec3 localMax{};
    if (!model.HasLocalBounds()) {
        return true;
    }
    model.GetLocalBounds(localMin, localMax);

    glm::vec3 worldMin{};
    glm::vec3 worldMax{};
    TransformBounds(model.GetWorldTransform(), localMin, localMax, worldMin, worldMax);
    for (const FrustumPlane& plane : frustumPlanes) {
        if (IsAabbOutsidePlane(plane, worldMin, worldMax)) {
            return false;
        }
    }
    return true;
}
} // namespace

Scene::Scene() : p_SceneImporter(CreateDefaultSceneImporter()) {
    InitializeDefaultLighting();
}

Scene::~Scene() = default;

void Scene::InitializeDefaultLighting() {
    m_Lights.clear();

    SceneLight light{};
    if (const auto config = Engine::Get().GetConfig()) {
        light.position = config->environment.lightPosition;
    }
    m_Lights.push_back(light);
}

void Scene::SetImporter(std::unique_ptr<ISceneImporter> importer) {
    if (!importer) {
        LOGW("Scene::SetImporter received null importer, keeping current importer");
        return;
    }
    p_SceneImporter = std::move(importer);
}

bool Scene::Load(const uint8_t* data, const size_t size, bool append) {
    if (!p_SceneImporter) {
        LOGE("Scene has no importer configured");
        return false;
    }
    ImportedSceneData sceneData{};
    if (!p_SceneImporter->LoadFromMemory(data, size, sceneData)) {
        return false;
    }
    return ProcessImportedScene(sceneData, append);
}

bool Scene::Load(const std::string& path, bool append) {
    if (!p_SceneImporter) {
        LOGE("Scene has no importer configured");
        return false;
    }
    ImportedSceneData sceneData{};
    if (!p_SceneImporter->LoadFromFile(path, sceneData)) {
        return false;
    }
    return ProcessImportedScene(sceneData, append);
}

bool Scene::ProcessImportedScene(const ImportedSceneData& sceneData, bool append) {
    if (!append) {
        m_Models.clear();
    }
    m_VisibleModels.clear();

    if (sceneData.rootNodeIndex < 0 || sceneData.rootNodeIndex >= static_cast<int>(sceneData.nodes.size())) {
        LOGE("Imported scene has invalid root node index: %d", sceneData.rootNodeIndex);
        return false;
    }

    if (ProcessImportedNode(sceneData.rootNodeIndex, sceneData, -1)) {
        LOGI("%s scene: %s",
             append ? "Successfully appended" : "Successfully processed",
             sceneData.nodes[sceneData.rootNodeIndex].name.c_str());
        return true;
    }
    return false;
}

bool Scene::ProcessImportedNode(int nodeIndex, const ImportedSceneData& sceneData, int parentIndex) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(sceneData.nodes.size())) {
        LOGE("Imported scene node index %d is out of range", nodeIndex);
        return false;
    }
    const ImportedNode& node = sceneData.nodes[nodeIndex];
    LOGD("Processing node: %s (%d meshes), parent index: %d",
         node.name.c_str(),
         static_cast<int>(node.meshIndices.size()),
         parentIndex);

    Model newModel{};
    newModel.Init(this, node.name, node.localTransform);
    for (int meshIndex : node.meshIndices) {
        if (meshIndex < 0 || meshIndex >= static_cast<int>(sceneData.meshes.size())) {
            LOGW("Skipping invalid imported mesh index %d in node %s", meshIndex, node.name.c_str());
            continue;
        }
        const ImportedMesh& mesh = sceneData.meshes[meshIndex];
        if (!mesh.vertices.empty() && !mesh.indices.empty()) {
            ProcessImportedMesh(mesh, sceneData, newModel.m_Meshes, node.name);
            newModel.m_Meshes.back().Init();
        }
    }
    newModel.RecomputeLocalBounds();

    const int currentNodeIndex = static_cast<int>(m_Models.size());
    if (parentIndex != -1) {
        newModel.SetParent(parentIndex);
        m_Models[parentIndex].m_Children.push_back(currentNodeIndex);
    }
    m_Models.emplace_back(std::move(newModel));

    for (int childNodeIndex : node.childNodeIndices) {
        if (!ProcessImportedNode(childNodeIndex, sceneData, currentNodeIndex)) {
            return false;
        }
    }
    return true;
}

void Scene::ProcessImportedMesh(const ImportedMesh& mesh,
                                const ImportedSceneData& sceneData,
                                std::vector<Mesh>& meshes,
                                const std::string& modelName) {
    std::vector<Vertex> vertices = mesh.vertices;
    std::vector<unsigned int> indices = mesh.indices;
    std::vector<Texture> textures;

    if (mesh.materialIndex.has_value()) {
        const int materialIndex = mesh.materialIndex.value();
        if (materialIndex >= 0 && materialIndex < static_cast<int>(sceneData.materials.size())) {
            LOGI("Processing material %d for model %s", materialIndex, modelName.c_str());
            const ImportedMaterial& material = sceneData.materials[materialIndex];
            if (!material.diffuseTexturePath.empty()) {
                std::filesystem::path texturePath = material.diffuseTexturePath;
                if (texturePath.is_relative() && !sceneData.sourceDirectory.empty()) {
                    texturePath = std::filesystem::path(sceneData.sourceDirectory) / texturePath;
                }

                Texture texture;
                if (texture.LoadFromFile(texturePath.string().c_str())) {
                    textures.emplace_back(std::move(texture));
                } else {
                    LOGW("Failed to load diffuse texture '%s' for model %s", texturePath.string().c_str(), modelName.c_str());
                }
            }
        }
    }

    meshes.emplace_back(std::move(vertices), std::move(indices), std::move(textures));
}

std::vector<int>& Scene::GetVisibleModels() {
    return m_VisibleModels;
}

std::vector<SceneLight>& Scene::GetLights() {
    return m_Lights;
}

const std::vector<SceneLight>& Scene::GetLights() const {
    return m_Lights;
}

std::vector<LightSnapshot> Scene::BuildLightSnapshots() const {
    std::vector<LightSnapshot> snapshots;
    snapshots.reserve(m_Lights.size());
    for (const SceneLight& light : m_Lights) {
        snapshots.push_back(light.ToSnapshot());
    }
    return snapshots;
}

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

    const auto frustumPlanes = ExtractFrustumPlanes(camera.m_ProjMat * camera.m_ViewMat);
    for (size_t i = 0; i < m_Models.size(); i++) {
        if (!IsModelVisibleInFrustum(m_Models[i], frustumPlanes)) {
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
