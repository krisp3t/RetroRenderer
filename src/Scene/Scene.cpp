#include "Scene.h"
#include "ISceneImporter.h"
#include "../Engine.h"
#include <KrisLogger/Logger.h>
#include <utility>

namespace RetroRenderer {
Scene::Scene() : p_SceneImporter(CreateDefaultSceneImporter()) {
}

Scene::~Scene() = default;

void Scene::SetImporter(std::unique_ptr<ISceneImporter> importer) {
    if (!importer) {
        LOGW("Scene::SetImporter received null importer, keeping current importer");
        return;
    }
    p_SceneImporter = std::move(importer);
}

bool Scene::Load(const uint8_t* data, const size_t size) {
    if (!p_SceneImporter) {
        LOGE("Scene has no importer configured");
        return false;
    }
    ImportedSceneData sceneData{};
    if (!p_SceneImporter->LoadFromMemory(data, size, sceneData)) {
        return false;
    }
    return ProcessImportedScene(sceneData);
}

bool Scene::Load(const std::string& path) {
    if (!p_SceneImporter) {
        LOGE("Scene has no importer configured");
        return false;
    }
    ImportedSceneData sceneData{};
    if (!p_SceneImporter->LoadFromFile(path, sceneData)) {
        return false;
    }
    return ProcessImportedScene(sceneData);
}

bool Scene::ProcessImportedScene(const ImportedSceneData& sceneData) {
    m_Models.clear();
    m_VisibleModels.clear();

    if (sceneData.rootNodeIndex < 0 || sceneData.rootNodeIndex >= static_cast<int>(sceneData.nodes.size())) {
        LOGE("Imported scene has invalid root node index: %d", sceneData.rootNodeIndex);
        return false;
    }

    if (ProcessImportedNode(sceneData.rootNodeIndex, sceneData, -1)) {
        LOGI("Successfully processed scene: %s", sceneData.nodes[sceneData.rootNodeIndex].name.c_str());
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
            (void)material; // TODO: process material
        }
    }

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
