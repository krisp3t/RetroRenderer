#include "ISceneImporter.h"

#include "LightweightObjSceneImporter.h"
#if RETRO_WITH_ASSIMP
#include "AssimpSceneImporter.h"
#endif
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
        if (m_ObjImporter.LoadFromMemory(data, size, outSceneData)) {
            return true;
        }
#if RETRO_WITH_ASSIMP
        return m_AssimpImporter.LoadFromMemory(data, size, outSceneData);
#else
        LOGE("Failed to load scene from memory as OBJ and RETRO_WITH_ASSIMP=0");
        return false;
#endif
    }

    bool LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) override {
        if (HasObjExtension(path)) {
            if (m_ObjImporter.LoadFromFile(path, outSceneData)) {
                return true;
            }
#if RETRO_WITH_ASSIMP
            LOGW("OBJ importer failed for '%s', trying Assimp fallback", path.c_str());
#else
            return false;
#endif
        }

#if RETRO_WITH_ASSIMP
        if (m_AssimpImporter.LoadFromFile(path, outSceneData)) {
            return true;
        }
#endif
        return m_ObjImporter.LoadFromFile(path, outSceneData);
    }

  private:
    LightweightObjSceneImporter m_ObjImporter;
#if RETRO_WITH_ASSIMP
    AssimpSceneImporter m_AssimpImporter;
#endif
};
} // namespace

std::unique_ptr<ISceneImporter> CreateDefaultSceneImporter() {
    return std::make_unique<DefaultSceneImporter>();
}

} // namespace RetroRenderer
