#pragma once

#include "../Base/Color.h"
#include "../Base/Config.h"
#include "../Scene/Camera.h"
#include "../Scene/Light.h"
#include "../Scene/Texture.h"
#include "../Scene/Vertex.h"
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

struct RenderMeshSnapshot {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct RenderItem {
    std::shared_ptr<const RenderMeshSnapshot> mesh;
    glm::mat4 worldTransform = glm::mat4(1.0f);
    FrameMaterialId materialId = kInvalidFrameMaterialId;
    FrameTextureId textureId = kInvalidFrameTextureId;
};

// Renderer input packet. It is intentionally detached from live Scene state so
// renderer work can safely run later or on another thread.
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
};

} // namespace RetroRenderer
