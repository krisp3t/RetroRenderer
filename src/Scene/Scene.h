#pragma once

#include "Camera.h"
#include "ImportedSceneData.h"
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

    bool Load(const uint8_t* data, const size_t size);
    bool Load(const std::string& path);
    void SetImporter(std::unique_ptr<ISceneImporter> importer);
    void FrustumCull(const Camera& camera);
    [[nodiscard]] std::vector<int>& GetVisibleModels();
    [[nodiscard]] const glm::mat4& GetModelWorldTransform(int index) const;
    void MarkDirtyModel(int index);

    std::vector<Model> m_Models;

  private:
    bool ProcessImportedScene(const ImportedSceneData& sceneData);
    bool ProcessImportedNode(int nodeIndex, const ImportedSceneData& sceneData, int parentIndex);
    void ProcessImportedMesh(const ImportedMesh& mesh,
                             const ImportedSceneData& sceneData,
                             std::vector<Mesh>& meshes,
                             const std::string& modelName);

    std::unique_ptr<ISceneImporter> p_SceneImporter;
    std::vector<int> m_VisibleModels;
};
} // namespace RetroRenderer
