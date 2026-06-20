#pragma once

#include "../Base/Color.h"
#include "../Base/Config.h"
#include "../Scene/Camera.h"
#include "../Scene/Light.h"
#include "../Scene/Mesh.h"
#include "../Scene/Texture.h"
#include "MaterialTypes.h"
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
    std::shared_ptr<const CompiledMaterialTemplate> compiledTemplate;
    std::vector<glm::vec4> parameterValues;
    std::vector<FrameTextureId> textureIds;
    MaterialPipelineState pipelineState{};
};

struct RenderItem {
    std::shared_ptr<const MeshGeometryData> geometry;
    glm::mat4 worldTransform = glm::mat4(1.0f);
    FrameMaterialId materialId = kInvalidFrameMaterialId;
};

// Renderer input packet. Per-frame state is copied, while immutable shared
// geometry and textures are referenced directly to avoid duplicate storage.
struct RenderPacket {
    bool hasScene = false;
    Camera camera;
    std::vector<LightSnapshot> lights;
    std::vector<FrameMaterialState> materials;
    std::vector<std::shared_ptr<const Texture>> textures;
    std::vector<RenderItem> items;
    Config configSnapshot{};
    Color clearColor{};
    uint64_t dataRevision = 0;
    uint64_t sceneResourceRevision = 0;
    uint64_t textureResourceRevision = 0;
};

} // namespace RetroRenderer
