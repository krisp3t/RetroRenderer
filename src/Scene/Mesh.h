#pragma once
#include <vector>
#include "Vertex.h"
#include "Texture.h"

#ifdef __ANDROID__
#include <GLES3/gl3.h> // For OpenGL ES 3.0
#else
#include <glad/glad.h>
#endif

namespace RetroRenderer
{

    class Mesh
    {
    public:
        ~Mesh() = default;
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

        void Init();

        // Per-vertex
        unsigned int m_numVertices = 0;
        std::vector<Vertex> m_Vertices;
        std::vector<unsigned int> m_Indices;

        // Per-face
        unsigned int m_numFaces = 0;

        // Per-mesh
        std::vector<Texture> m_Textures;

        // TODO: get rid of OpenGL specifics!!
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
    };


}
