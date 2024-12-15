#pragma once

#include <vector>
#include "../Renderer/Vertex.h"
#include "glad/glad.h"

namespace RetroRenderer
{

    class Mesh
    {
    public:
        ~Mesh() = default;
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

        void Init();

        // Per-vertex
        unsigned int m_numVertices = 0;
        std::vector<Vertex> m_Vertices;
        std::vector<unsigned int> m_Indices;

        // Per-face
        unsigned int m_numFaces = 0;
        // std::vector<Texture>

        // OpenGL specific
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
    };


}
