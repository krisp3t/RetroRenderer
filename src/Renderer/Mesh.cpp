#include "Mesh.h"

namespace RetroRenderer
{
    Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    {
        m_Vertices = vertices;
        m_Indices = indices;
        Init();
    }

    void Mesh::Init()
    {

    }
}