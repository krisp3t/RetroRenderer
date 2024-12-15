#include "Mesh.h"

namespace RetroRenderer
{
    Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    {
        m_Vertices = vertices;
        m_Indices = indices;
        m_numVertices = vertices.size();
        m_numFaces = indices.size() / 3;
        Init();
    }

    void Mesh::Init()
    {

    }
}