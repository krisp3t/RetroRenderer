#include "Mesh.h"
#include <utility>

namespace RetroRenderer {
Mesh::Mesh(std::vector<Vertex> vertices,
           std::vector<unsigned int> indices,
           std::vector<Texture> textures,
           SceneMaterialHandle materialHandle)
    : m_Vertices(std::move(vertices)),
      m_Indices(std::move(indices)),
      m_Textures(std::move(textures)),
      m_MaterialHandle(materialHandle) {
    m_numVertices = m_Vertices.size();
    m_numFaces = m_Indices.size() / 3;
}

const std::vector<Vertex>& Mesh::GetVertices() const {
    return m_Vertices;
}

const std::vector<unsigned int>& Mesh::GetIndices() const {
    return m_Indices;
}

const std::vector<Texture>& Mesh::GetTextures() const {
    return m_Textures;
}

const Texture* Mesh::GetPrimaryTexture() const {
    if (m_Textures.empty()) {
        return nullptr;
    }
    return &m_Textures.front();
}

SceneMaterialHandle Mesh::GetMaterialHandle() const {
    return m_MaterialHandle;
}

unsigned int Mesh::GetVertexCount() const {
    return m_numVertices;
}

unsigned int Mesh::GetFaceCount() const {
    return m_numFaces;
}
} // namespace RetroRenderer
