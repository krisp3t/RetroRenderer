#pragma once

#include "Camera.h"
#include "ImportedSceneData.h"
#include "Light.h"
#include "Mesh.h"
#include "Model.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace RetroRenderer {
class ISceneImporter;

class Scene {
  public:
    Scene();
    ~Scene();

    bool Load(const uint8_t* data, const size_t size, bool append = false);
    bool Load(const std::string& path, bool append = false);
    void SetImporter(std::unique_ptr<ISceneImporter> importer);
    void FrustumCull(const Camera& camera);
    [[nodiscard]] std::vector<int>& GetVisibleModels();
    [[nodiscard]] std::vector<SceneLight>& GetLights();
    [[nodiscard]] const std::vector<SceneLight>& GetLights() const;
    [[nodiscard]] std::vector<LightSnapshot> BuildLightSnapshots() const;
    [[nodiscard]] const glm::mat4& GetModelWorldTransform(int index) const;
    void MarkDirtyModel(int index);

    std::vector<Model> m_Models;
    std::vector<SceneLight> m_Lights;

  private:
    void InitializeDefaultLighting();
    bool ProcessImportedScene(const ImportedSceneData& sceneData, bool append);
    bool ProcessImportedNode(int nodeIndex, const ImportedSceneData& sceneData, int parentIndex);
    void ProcessImportedMesh(const ImportedMesh& mesh,
                             const ImportedSceneData& sceneData,
                             std::vector<Mesh>& meshes,
                             const std::string& modelName);

    std::unique_ptr<ISceneImporter> p_SceneImporter;
    std::vector<int> m_VisibleModels;
};
} // namespace RetroRenderer
