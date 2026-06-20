#pragma once

#include "../Base/Config.h"
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
    void SetDefaultLightPosition(const glm::vec3& lightPosition);
    void FrustumCull(const Camera& camera, const Config::CullSettings& cullSettings);
    [[nodiscard]] std::vector<int>& GetVisibleModels();
    [[nodiscard]] std::vector<SceneLight>& GetLights();
    [[nodiscard]] const std::vector<SceneLight>& GetLights() const;
    void BuildLightSnapshots(std::vector<LightSnapshot>& outSnapshots) const;
    [[nodiscard]] const glm::mat4& GetModelWorldTransform(int index) const;
    void MarkDirtyModel(int index);
    [[nodiscard]] Model& GetModel(size_t index);
    [[nodiscard]] const Model& GetModel(size_t index) const;
    [[nodiscard]] size_t GetModelCount() const;

  private:
    void InitializeDefaultLighting(const glm::vec3& lightPosition);
    bool ProcessImportedScene(const ImportedSceneData& sceneData, bool append);
    bool ProcessImportedNode(int nodeIndex, const ImportedSceneData& sceneData, int parentIndex);
    void ProcessImportedMesh(const ImportedMesh& mesh,
                             const ImportedSceneData& sceneData,
                             std::vector<Mesh>& meshes,
                             const std::string& modelName);

    std::unique_ptr<ISceneImporter> p_SceneImporter;
    std::vector<int> m_VisibleModels;
    std::vector<Model> m_Models;
    std::vector<SceneLight> m_Lights;
};
} // namespace RetroRenderer
