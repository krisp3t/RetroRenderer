#include "Mesh.h"
#include <utility>

namespace RetroRenderer {
Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    : m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_Textures(std::move(textures)) {
    m_numVertices = m_Vertices.size();
    m_numFaces = m_Indices.size() / 3;
}

void Mesh::Init() {
    // TODO: don't assume gl
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Vertex data -> VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_numVertices * sizeof(Vertex), m_Vertices.data(), GL_STATIC_DRAW);

    // Index data -> EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), m_Indices.data(), GL_STATIC_DRAW);

    // Vertex attribute pointers
    // Position
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    // TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(2);
    // Color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);

#ifndef NDEBUG
    {
        GLint prev = 0;
        GLint sizeBytes = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &sizeBytes);
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prev));
        assert(sizeBytes >= static_cast<GLint>(m_numVertices * sizeof(Vertex)) && "VBO is smaller than expected");
    }
    {
        GLint prev = 0;
        GLint sizeBytes = 0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &sizeBytes);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(prev));
        assert(sizeBytes >= static_cast<GLint>(m_Indices.size() * sizeof(unsigned int)) &&
               "EBO is smaller than expected");
    }
#endif

    glBindVertexArray(0);
}
} // namespace RetroRenderer