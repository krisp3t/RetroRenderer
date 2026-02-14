#pragma once

#include "ISceneImporter.h"

struct aiScene;

namespace RetroRenderer {

class AssimpSceneImporter final : public ISceneImporter {
  public:
    bool LoadFromMemory(const uint8_t* data, size_t size, ImportedSceneData& outSceneData) override;
    bool LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) override;

  private:
    bool BuildImportedSceneData(const aiScene* scene, ImportedSceneData& outSceneData) const;
};

} // namespace RetroRenderer
