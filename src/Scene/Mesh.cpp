#include "Mesh.h"
#include <utility>

namespace RetroRenderer {
namespace {
const std::vector<Vertex>& EmptyVertices() {
    static const std::vector<Vertex> empty;
    return empty;
}

const std::vector<unsigned int>& EmptyIndices() {
    static const std::vector<unsigned int> empty;
    return empty;
}
} // namespace

uint64_t MeshGeometryData::EstimateResidentCpuBytes() const {
    return sizeof(MeshGeometryData) + vertices.capacity() * sizeof(Vertex) + indices.capacity() * sizeof(unsigned int);
}

Mesh::Mesh(std::vector<Vertex> vertices,
           std::vector<unsigned int> indices,
           SceneMaterialHandle materialHandle)
    : m_Geometry(std::make_shared<MeshGeometryData>(MeshGeometryData{
          .vertices = std::move(vertices),
          .indices = std::move(indices),
      })),
      m_MaterialHandle(materialHandle) {
}

Mesh::Mesh(std::shared_ptr<const MeshGeometryData> geometry, SceneMaterialHandle materialHandle)
    : m_Geometry(std::move(geometry)),
      m_MaterialHandle(materialHandle) {
}

const std::vector<Vertex>& Mesh::GetVertices() const {
    return m_Geometry ? m_Geometry->vertices : EmptyVertices();
}

const std::vector<unsigned int>& Mesh::GetIndices() const {
    return m_Geometry ? m_Geometry->indices : EmptyIndices();
}

const std::shared_ptr<const MeshGeometryData>& Mesh::GetGeometry() const {
    return m_Geometry;
}

SceneMaterialHandle Mesh::GetMaterialHandle() const {
    return m_MaterialHandle;
}

unsigned int Mesh::GetVertexCount() const {
    return static_cast<unsigned int>(GetVertices().size());
}

unsigned int Mesh::GetFaceCount() const {
    return static_cast<unsigned int>(GetIndices().size() / 3);
}
} // namespace RetroRenderer
