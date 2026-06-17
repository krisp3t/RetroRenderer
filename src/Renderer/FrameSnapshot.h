#pragma once

#include "../Base/Color.h"
#include "../Base/Config.h"
#include "../Scene/Camera.h"
#include "../Scene/Light.h"
#include "../Scene/Scene.h"
#include "../Scene/Texture.h"
#include "ShaderHandle.h"
#include <glm/mat4x4.hpp>
#include <limits>
#include <memory>
#include <vector>

namespace RetroRenderer {

using FrameMaterialId = uint32_t;
using FrameTextureId = uint32_t;
constexpr FrameMaterialId kInvalidFrameMaterialId = std::numeric_limits<FrameMaterialId>::max();
constexpr FrameTextureId kInvalidFrameTextureId = std::numeric_limits<FrameTextureId>::max();

struct FrameMaterialState {
    ShaderHandle shaderHandle;
    glm::vec3 lightColor = glm::vec3(1.0f);
    bool useVertexColor = false;
    bool enablePhong = false;
    float ambientStrength = 0.3f;
    float specularStrength = 0.3f;
    float shininess = 32.0f;
};

struct FrameTextureBinding {
    enum class Source : uint8_t {
        None,
        Scene,
        Override,
    };

    Source source = Source::None;
    const Texture* texture = nullptr;

    [[nodiscard]] bool IsValid() const {
        return texture != nullptr;
    }
};

struct RenderItem {
    int modelIndex = -1;
    int meshIndex = -1;
    glm::mat4 worldTransform = glm::mat4(1.0f);
    FrameMaterialId materialId = kInvalidFrameMaterialId;
    FrameTextureId textureId = kInvalidFrameTextureId;
};

struct FrameSnapshot {
    std::shared_ptr<const Scene> scene;
    Camera camera;
    std::vector<LightSnapshot> lights;
    std::vector<FrameMaterialState> materials;
    std::vector<FrameTextureBinding> textures;
    std::vector<RenderItem> items;
    Config configSnapshot{};
    Color clearColor{};
    uint64_t dataRevision = 0;
};

} // namespace RetroRenderer
