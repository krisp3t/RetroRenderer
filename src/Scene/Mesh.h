#pragma once
#include "../include/kris_glheaders.h"
#include "Texture.h"
#include "Vertex.h"
#include <vector>

namespace RetroRenderer {

class Mesh {
  public:
    Mesh() = default;
    ~Mesh() = default;
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    Mesh(Mesh &&) noexcept = default;
    Mesh &operator=(Mesh &&) noexcept = default;

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

} // namespace RetroRenderer
