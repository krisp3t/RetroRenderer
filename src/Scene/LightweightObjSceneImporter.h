#pragma once

#include "ISceneImporter.h"

namespace RetroRenderer {

class LightweightObjSceneImporter final : public ISceneImporter {
  public:
    bool LoadFromMemory(const uint8_t* data, size_t size, ImportedSceneData& outSceneData) override;
    bool LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) override;
};

} // namespace RetroRenderer
