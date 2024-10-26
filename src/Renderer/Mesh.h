#pragma once
#include <vector>
#include "Vertex.h"

namespace RetroRenderer
{

    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

        void Init();

        std::vector<Vertex> m_Vertices;
        std::vector<unsigned int> m_Indices;
        // std::vector<Texture>
    private:
        unsigned int VAO, VBO, EBO;
    };



}
