#pragma once
#include "../Renderer/MaterialTypes.h"
#include "Vertex.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace RetroRenderer {

struct MeshGeometryData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    [[nodiscard]] uint64_t EstimateResidentCpuBytes() const;
};

class Mesh {
  public:
    Mesh() = default;
    ~Mesh() = default;
    Mesh(std::vector<Vertex> vertices,
         std::vector<unsigned int> indices,
         SceneMaterialHandle materialHandle = kInvalidSceneMaterialHandle);
    Mesh(std::shared_ptr<const MeshGeometryData> geometry,
         SceneMaterialHandle materialHandle = kInvalidSceneMaterialHandle);
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    [[nodiscard]] const std::vector<Vertex>& GetVertices() const;
    [[nodiscard]] const std::vector<unsigned int>& GetIndices() const;
    [[nodiscard]] const std::shared_ptr<const MeshGeometryData>& GetGeometry() const;
    [[nodiscard]] SceneMaterialHandle GetMaterialHandle() const;
    [[nodiscard]] unsigned int GetVertexCount() const;
    [[nodiscard]] unsigned int GetFaceCount() const;

  private:
    std::shared_ptr<const MeshGeometryData> m_Geometry;
    SceneMaterialHandle m_MaterialHandle = kInvalidSceneMaterialHandle;
};

} // namespace RetroRenderer
