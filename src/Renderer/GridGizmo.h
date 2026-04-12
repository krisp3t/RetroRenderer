#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace RetroRenderer {
struct GridGizmoVertex {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
};

[[nodiscard]] std::vector<GridGizmoVertex> BuildGridGizmoVertices(const glm::vec3& cameraPosition);
} // namespace RetroRenderer
