#pragma once

#include "ImportedSceneData.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace RetroRenderer {

class ISceneImporter {
  public:
    virtual ~ISceneImporter() = default;

    virtual bool LoadFromMemory(const uint8_t* data, size_t size, ImportedSceneData& outSceneData) = 0;
    virtual bool LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) = 0;
};

std::unique_ptr<ISceneImporter> CreateDefaultSceneImporter();

} // namespace RetroRenderer
