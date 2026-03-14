#pragma once

#include <glm/glm.hpp>
#include <string>

namespace RetroRenderer {

enum class LightType {
    POINT,
};

struct LightSnapshot {
    LightType type = LightType::POINT;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
};

struct SceneLight {
    std::string name = "Main light";
    LightType type = LightType::POINT;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;

    [[nodiscard]] LightSnapshot ToSnapshot() const {
        return LightSnapshot{
            .type = type,
            .position = position,
            .color = color,
            .intensity = intensity,
        };
    }
};

} // namespace RetroRenderer
