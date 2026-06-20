#include "Scene.h"
#include "ISceneImporter.h"
#include "../Base/Config.h"
#include <KrisLogger/Logger.h>
#include <algorithm>
#include <array>
#include <cmath>
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

MaterialValue MakeMaterialValue(MaterialDataType type, const glm::vec4& data) {
    return MaterialValue{.type = type, .data = data};
}

MaterialParameterOverride MakeMaterialParameter(const std::string& name, MaterialDataType type, const glm::vec4& data) {
    return MaterialParameterOverride{
        .name = name,
        .value = MakeMaterialValue(type, data),
    };
}
} // namespace

Scene::Scene() : p_SceneImporter(CreateDefaultSceneImporter()) {
    InitializeDefaultLighting(glm::vec3(0.0f, 0.0f, 5.0f));
}

Scene::~Scene() = default;

void Scene::InitializeDefaultLighting(const glm::vec3& lightPosition) {
    m_Lights.clear();

    SceneLight light{};
    light.position = lightPosition;
    m_Lights.push_back(light);
}

void Scene::SetDefaultLightPosition(const glm::vec3& lightPosition) {
    if (m_Lights.empty()) {
        InitializeDefaultLighting(lightPosition);
        return;
    }
    m_Lights.front().position = lightPosition;
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
        m_Materials.clear();
    }
    m_VisibleModels.clear();

    if (sceneData.rootNodeIndex < 0 || sceneData.rootNodeIndex >= static_cast<int>(sceneData.nodes.size())) {
        LOGE("Imported scene has invalid root node index: %d", sceneData.rootNodeIndex);
        return false;
    }

    std::vector<SceneMaterialHandle> importedMaterialHandles;
    importedMaterialHandles.reserve(sceneData.materials.size());
    for (size_t materialIndex = 0; materialIndex < sceneData.materials.size(); materialIndex++) {
        bool preferVertexColor = false;
        for (const ImportedMesh& mesh : sceneData.meshes) {
            if (mesh.materialIndex.has_value() && mesh.materialIndex.value() == static_cast<int>(materialIndex) && MeshUsesVertexColor(mesh)) {
                preferVertexColor = true;
                break;
            }
        }
        importedMaterialHandles.push_back(
            AppendImportedMaterial(sceneData.materials[materialIndex],
                                   sceneData.sourceDirectory,
                                   preferVertexColor,
                                   sceneData.materials[materialIndex].name.empty()
                                       ? "Imported material " + std::to_string(materialIndex)
                                       : sceneData.materials[materialIndex].name));
    }

    if (ProcessImportedNode(sceneData.rootNodeIndex, sceneData, importedMaterialHandles, -1)) {
        LOGI("%s scene: %s",
             append ? "Successfully appended" : "Successfully processed",
             sceneData.nodes[sceneData.rootNodeIndex].name.c_str());
        return true;
    }
    return false;
}

bool Scene::ProcessImportedNode(int nodeIndex,
                                const ImportedSceneData& sceneData,
                                const std::vector<SceneMaterialHandle>& importedMaterialHandles,
                                int parentIndex) {
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
            ProcessImportedMesh(mesh, sceneData, importedMaterialHandles, newModel.m_Meshes, node.name);
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
        if (!ProcessImportedNode(childNodeIndex, sceneData, importedMaterialHandles, currentNodeIndex)) {
            return false;
        }
    }
    return true;
}

void Scene::ProcessImportedMesh(const ImportedMesh& mesh,
                                const ImportedSceneData&,
                                const std::vector<SceneMaterialHandle>& importedMaterialHandles,
                                std::vector<Mesh>& meshes,
                                const std::string& modelName) {
    std::vector<Vertex> vertices = mesh.vertices;
    std::vector<unsigned int> indices = mesh.indices;
    std::vector<Texture> textures;
    SceneMaterialHandle materialHandle = kInvalidSceneMaterialHandle;

    if (mesh.materialIndex.has_value()) {
        const int materialIndex = mesh.materialIndex.value();
        if (materialIndex >= 0 && materialIndex < static_cast<int>(importedMaterialHandles.size())) {
            materialHandle = importedMaterialHandles[static_cast<size_t>(materialIndex)];
            LOGI("Processing material %d for model %s", materialIndex, modelName.c_str());
            if (const SceneMaterial* sceneMaterial = GetMaterial(materialHandle);
                sceneMaterial != nullptr && !sceneMaterial->textureBindings.empty()) {
                textures.emplace_back(sceneMaterial->textureBindings.front().texture.CloneCpuOnly());
            }
        }
    }

    if (materialHandle == kInvalidSceneMaterialHandle) {
        materialHandle = GetOrCreateFallbackMaterial(MeshUsesVertexColor(mesh));
    }

    meshes.emplace_back(std::move(vertices), std::move(indices), std::move(textures), materialHandle);
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

void Scene::BuildLightSnapshots(std::vector<LightSnapshot>& outSnapshots) const {
    outSnapshots.clear();
    outSnapshots.reserve(m_Lights.size());
    for (const SceneLight& light : m_Lights) {
        outSnapshots.push_back(light.ToSnapshot());
    }
}

void Scene::FrustumCull(const Camera& camera, const Config::CullSettings& cullSettings) {
    m_VisibleModels.clear();
    m_VisibleModels.reserve(m_Models.size());

    if (!cullSettings.frustumCull) {
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
    const size_t modelIndex = static_cast<size_t>(index);
    assert(index >= 0 && modelIndex < m_Models.size() && "Invalid model index");
    return m_Models[modelIndex].GetWorldTransform();
}

void Scene::MarkDirtyModel(int index) {
    m_Models[index].MarkDirty();
}

Model& Scene::GetModel(size_t index) {
    return m_Models.at(index);
}

const Model& Scene::GetModel(size_t index) const {
    return m_Models.at(index);
}

size_t Scene::GetModelCount() const {
    return m_Models.size();
}

size_t Scene::GetMaterialCount() const {
    return m_Materials.size();
}

SceneMaterial* Scene::GetMaterial(SceneMaterialHandle handle) {
    if (handle == kInvalidSceneMaterialHandle || handle >= m_Materials.size()) {
        return nullptr;
    }
    return &m_Materials[handle];
}

const SceneMaterial* Scene::GetMaterial(SceneMaterialHandle handle) const {
    if (handle == kInvalidSceneMaterialHandle || handle >= m_Materials.size()) {
        return nullptr;
    }
    return &m_Materials[handle];
}

std::vector<SceneMaterial>& Scene::GetMaterials() {
    return m_Materials;
}

const std::vector<SceneMaterial>& Scene::GetMaterials() const {
    return m_Materials;
}

void Scene::SetAllMaterialTemplates(const std::filesystem::path& templatePath) {
    for (SceneMaterial& material : m_Materials) {
        material.templatePath = templatePath;
    }
}

SceneMaterialHandle Scene::AppendImportedMaterial(const ImportedMaterial& material,
                                                  const std::string& sourceDirectory,
                                                  bool preferVertexColor,
                                                  const std::string& name) {
    SceneMaterial sceneMaterial{};
    sceneMaterial.name = name;
    if (!material.diffuseTexturePath.empty()) {
        sceneMaterial.templatePath = kMaterialAssetPhongTextured;
    } else if (preferVertexColor) {
        sceneMaterial.templatePath = kMaterialAssetPhongVertexColor;
    } else {
        sceneMaterial.templatePath = kMaterialAssetPhongConstant;
    }

    const float specularStrength = std::max({material.specularColor.r, material.specularColor.g, material.specularColor.b});
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("tint", MaterialDataType::VEC4, glm::vec4(material.diffuseColor, material.alpha)));
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("baseColor", MaterialDataType::VEC4, glm::vec4(material.diffuseColor, material.alpha)));
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("ambientStrength", MaterialDataType::FLOAT1, glm::vec4(0.3f, 0.0f, 0.0f, 0.0f)));
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("specularStrength", MaterialDataType::FLOAT1, glm::vec4(specularStrength, 0.0f, 0.0f, 0.0f)));
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("shininess", MaterialDataType::FLOAT1, glm::vec4(material.shininess, 0.0f, 0.0f, 0.0f)));

    if (material.alpha < 0.999f) {
        sceneMaterial.pipelineOverrides.blendMode = MaterialBlendMode::ALPHA_BLEND;
    }

    if (!material.diffuseTexturePath.empty()) {
        std::filesystem::path texturePath = material.diffuseTexturePath;
        if (texturePath.is_relative() && !sourceDirectory.empty()) {
            texturePath = std::filesystem::path(sourceDirectory) / texturePath;
        }

        Texture texture;
        if (texture.LoadFromFile(texturePath.string().c_str())) {
            sceneMaterial.textureBindings.push_back(MaterialTextureBinding{
                .slotName = "albedo",
                .texture = std::move(texture),
            });
        } else {
            LOGW("Failed to load diffuse texture '%s' for material %s", texturePath.string().c_str(), name.c_str());
        }
    }

    const SceneMaterialHandle handle = static_cast<SceneMaterialHandle>(m_Materials.size());
    m_Materials.push_back(std::move(sceneMaterial));
    return handle;
}

SceneMaterialHandle Scene::GetOrCreateFallbackMaterial(bool preferVertexColor) {
    const std::filesystem::path templatePath = preferVertexColor ? std::filesystem::path(kMaterialAssetPhongVertexColor)
                                                                 : std::filesystem::path(kMaterialAssetPhongConstant);
    for (SceneMaterialHandle handle = 0; handle < m_Materials.size(); handle++) {
        if (m_Materials[handle].templatePath == templatePath &&
            m_Materials[handle].name == (preferVertexColor ? "Default vertex-color material" : "Default material")) {
            return handle;
        }
    }

    SceneMaterial sceneMaterial{};
    sceneMaterial.name = preferVertexColor ? "Default vertex-color material" : "Default material";
    sceneMaterial.templatePath = templatePath;
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("baseColor", MaterialDataType::VEC4, glm::vec4(1.0f)));
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("tint", MaterialDataType::VEC4, glm::vec4(1.0f)));
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("ambientStrength", MaterialDataType::FLOAT1, glm::vec4(0.3f, 0.0f, 0.0f, 0.0f)));
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("specularStrength", MaterialDataType::FLOAT1, glm::vec4(0.3f, 0.0f, 0.0f, 0.0f)));
    sceneMaterial.parameterOverrides.push_back(
        MakeMaterialParameter("shininess", MaterialDataType::FLOAT1, glm::vec4(32.0f, 0.0f, 0.0f, 0.0f)));

    const SceneMaterialHandle handle = static_cast<SceneMaterialHandle>(m_Materials.size());
    m_Materials.push_back(std::move(sceneMaterial));
    return handle;
}

bool Scene::MeshUsesVertexColor(const ImportedMesh& mesh) {
    for (const Vertex& vertex : mesh.vertices) {
        if (std::abs(vertex.color.r - 1.0f) > 1e-5f ||
            std::abs(vertex.color.g - 1.0f) > 1e-5f ||
            std::abs(vertex.color.b - 1.0f) > 1e-5f) {
            return true;
        }
    }
    return false;
}
} // namespace RetroRenderer
