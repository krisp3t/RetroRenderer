#pragma once

#include "../Base/Color.h"
#include "../Base/Config.h"
#include "../Scene/Camera.h"
#include "../Scene/Light.h"
#include "../Scene/Scene.h"
#include "../Scene/Texture.h"
#include "ShaderHandle.h"
#include <glm/mat4x4.hpp>
#include <memory>
#include <vector>

namespace RetroRenderer {

struct FrameMaterialState {
    ShaderHandle shaderHandle;
    glm::vec3 lightColor = glm::vec3(1.0f);
    bool useVertexColor = false;
    bool enablePhong = false;
    float ambientStrength = 0.3f;
    float specularStrength = 0.3f;
    float shininess = 32.0f;
    const Texture* textureOverride = nullptr;
};

struct RenderItem {
    int modelIndex = -1;
    int meshIndex = -1;
    glm::mat4 worldTransform = glm::mat4(1.0f);
};

struct FrameSnapshot {
    std::shared_ptr<const Scene> scene;
    Camera camera;
    std::vector<LightSnapshot> lights;
    std::vector<RenderItem> items;
    Config configSnapshot{};
    Color clearColor{};
    FrameMaterialState materialState{};
};

} // namespace RetroRenderer
