#include "GridGizmo.h"

namespace RetroRenderer {
std::vector<GridGizmoVertex> BuildGridGizmoVertices() {
    constexpr int kHalfExtent = 16;
    constexpr int kMajorLineStep = 5;
    constexpr float kGridHeight = 0.001f;
    const glm::vec3 minorColor(0.22f, 0.24f, 0.28f);
    const glm::vec3 majorColor(0.34f, 0.36f, 0.40f);
    const glm::vec3 xAxisColor(0.80f, 0.25f, 0.22f);
    const glm::vec3 zAxisColor(0.25f, 0.45f, 0.82f);

    std::vector<GridGizmoVertex> vertices;
    vertices.reserve(static_cast<size_t>(kHalfExtent * 4 + 2) * 4);

    for (int i = -kHalfExtent; i <= kHalfExtent; i++) {
        const float offset = static_cast<float>(i);
        const bool isMajor = i != 0 && (i % kMajorLineStep == 0);
        const glm::vec3 xAlignedColor = i == 0 ? xAxisColor : (isMajor ? majorColor : minorColor);
        const glm::vec3 zAlignedColor = i == 0 ? zAxisColor : (isMajor ? majorColor : minorColor);

        vertices.push_back({glm::vec3(-static_cast<float>(kHalfExtent), kGridHeight, offset), xAlignedColor});
        vertices.push_back({glm::vec3(static_cast<float>(kHalfExtent), kGridHeight, offset), xAlignedColor});
        vertices.push_back({glm::vec3(offset, kGridHeight, -static_cast<float>(kHalfExtent)), zAlignedColor});
        vertices.push_back({glm::vec3(offset, kGridHeight, static_cast<float>(kHalfExtent)), zAlignedColor});
    }

    return vertices;
}
} // namespace RetroRenderer
