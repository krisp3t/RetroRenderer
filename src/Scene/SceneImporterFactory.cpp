#include "ISceneImporter.h"

#include "LightweightObjSceneImporter.h"
#include <KrisLogger/Logger.h>
#include <cctype>
#include <memory>
#include <string>

namespace RetroRenderer {
namespace {
bool HasObjExtension(const std::string& path) {
    if (path.size() < 4) {
        return false;
    }
    const size_t extPos = path.size() - 4;
    return std::tolower(static_cast<unsigned char>(path[extPos])) == '.' &&
           std::tolower(static_cast<unsigned char>(path[extPos + 1])) == 'o' &&
           std::tolower(static_cast<unsigned char>(path[extPos + 2])) == 'b' &&
           std::tolower(static_cast<unsigned char>(path[extPos + 3])) == 'j';
}

class DefaultSceneImporter final : public ISceneImporter {
  public:
    bool LoadFromMemory(const uint8_t* data, size_t size, ImportedSceneData& outSceneData) override {
        const bool loaded = m_ObjImporter.LoadFromMemory(data, size, outSceneData);
        if (!loaded) {
            LOGE("Failed to load scene from memory. Only OBJ scenes are supported.");
        }
        return loaded;
    }

    bool LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) override {
        if (!HasObjExtension(path)) {
            LOGE("Unsupported scene format '%s'. Only OBJ scenes are supported.", path.c_str());
            return false;
        }

        const bool loaded = m_ObjImporter.LoadFromFile(path, outSceneData);
        if (!loaded) {
            LOGE("Failed to load OBJ scene '%s'", path.c_str());
        }
        return loaded;
    }

  private:
    LightweightObjSceneImporter m_ObjImporter;
};
} // namespace

std::unique_ptr<ISceneImporter> CreateDefaultSceneImporter() {
    return std::make_unique<DefaultSceneImporter>();
}

} // namespace RetroRenderer
