#include "Mesh.h"
#include <utility>

namespace RetroRenderer {
Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    : m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_Textures(std::move(textures)) {
    m_numVertices = m_Vertices.size();
    m_numFaces = m_Indices.size() / 3;
}
} // namespace RetroRenderer
