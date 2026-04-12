#include "GridGizmo.h"
#include <cmath>

namespace RetroRenderer {
std::vector<GridGizmoVertex> BuildGridGizmoVertices(const glm::vec3& cameraPosition) {
    constexpr int kHalfExtent = 64;
    constexpr int kMajorLineStep = 5;
    constexpr float kGridHeight = 0.001f;
    constexpr float kGridCellSize = 1.0f;
    const glm::vec3 minorColor(0.22f, 0.24f, 0.28f);
    const glm::vec3 majorColor(0.34f, 0.36f, 0.40f);
    const glm::vec3 xAxisColor(0.80f, 0.25f, 0.22f);
    const glm::vec3 zAxisColor(0.25f, 0.45f, 0.82f);
    const int centerX = static_cast<int>(std::floor(cameraPosition.x / kGridCellSize));
    const int centerZ = static_cast<int>(std::floor(cameraPosition.z / kGridCellSize));
    const float gridMinX = static_cast<float>(centerX - kHalfExtent) * kGridCellSize;
    const float gridMaxX = static_cast<float>(centerX + kHalfExtent) * kGridCellSize;
    const float gridMinZ = static_cast<float>(centerZ - kHalfExtent) * kGridCellSize;
    const float gridMaxZ = static_cast<float>(centerZ + kHalfExtent) * kGridCellSize;

    std::vector<GridGizmoVertex> vertices;
    vertices.reserve(static_cast<size_t>(kHalfExtent * 4 + 2) * 4);

    for (int i = -kHalfExtent; i <= kHalfExtent; i++) {
        const int worldZ = centerZ + i;
        const int worldX = centerX + i;
        const float offsetZ = static_cast<float>(worldZ) * kGridCellSize;
        const float offsetX = static_cast<float>(worldX) * kGridCellSize;
        const bool isMajorZ = worldZ != 0 && (worldZ % kMajorLineStep == 0);
        const bool isMajorX = worldX != 0 && (worldX % kMajorLineStep == 0);
        const glm::vec3 xAlignedColor = worldZ == 0 ? xAxisColor : (isMajorZ ? majorColor : minorColor);
        const glm::vec3 zAlignedColor = worldX == 0 ? zAxisColor : (isMajorX ? majorColor : minorColor);

        vertices.push_back({glm::vec3(gridMinX, kGridHeight, offsetZ), xAlignedColor});
        vertices.push_back({glm::vec3(gridMaxX, kGridHeight, offsetZ), xAlignedColor});
        vertices.push_back({glm::vec3(offsetX, kGridHeight, gridMinZ), zAlignedColor});
        vertices.push_back({glm::vec3(offsetX, kGridHeight, gridMaxZ), zAlignedColor});
    }

    return vertices;
}
} // namespace RetroRenderer
