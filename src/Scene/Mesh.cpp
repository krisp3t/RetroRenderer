#include <SDL_egl.h>

#include <utility>
#include "Mesh.h"

namespace RetroRenderer
{
    Mesh::Mesh(std::vector<Vertex> vertices,
     std::vector<unsigned int> indices,
     std::vector<Texture> textures)
    : m_Vertices(std::move(vertices))
    , m_Indices(std::move(indices))
    , m_Textures(std::move(textures))
    {
        m_numVertices = vertices.size();
        m_numFaces = indices.size() / 3;
        Init();
    }

    void Mesh::Init()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // Vertex data -> VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, m_numVertices * sizeof(Vertex), m_Vertices.data(), GL_STATIC_DRAW);

        // Index data -> EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), m_Indices.data(),
                     GL_STATIC_DRAW);

        // Vertex attribute pointers
        // Position
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);
        // TexCoords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, texCoords));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }
}