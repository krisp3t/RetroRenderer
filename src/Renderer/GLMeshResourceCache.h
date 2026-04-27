#pragma once

#include "../Scene/Mesh.h"
#include "../Scene/Vertex.h"
#include "../include/kris_glheaders.h"
#include <KrisLogger/Logger.h>
#include <cstddef>
#include <unordered_map>

namespace RetroRenderer {

class GLMeshResourceCache {
  public:
    struct MeshGpuResources {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLsizei indexCount = 0;
    };

    GLMeshResourceCache() = default;
    ~GLMeshResourceCache() = default;
    GLMeshResourceCache(const GLMeshResourceCache&) = delete;
    GLMeshResourceCache& operator=(const GLMeshResourceCache&) = delete;

    const MeshGpuResources& GetOrCreate(const Mesh& mesh) {
        auto it = m_Resources.find(&mesh);
        if (it != m_Resources.end()) {
            return it->second;
        }

        MeshGpuResources resources{};
        resources.indexCount = static_cast<GLsizei>(mesh.m_Indices.size());

        glGenVertexArrays(1, &resources.vao);
        glGenBuffers(1, &resources.vbo);
        glGenBuffers(1, &resources.ebo);

        glBindVertexArray(resources.vao);

        glBindBuffer(GL_ARRAY_BUFFER, resources.vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.m_Vertices.size() * sizeof(Vertex)),
            mesh.m_Vertices.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.m_Indices.size() * sizeof(unsigned int)),
            mesh.m_Indices.data(),
            GL_STATIC_DRAW);

        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoords)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        LOGD("Created GL mesh resources: %zu verts, %zu indices", mesh.m_Vertices.size(), mesh.m_Indices.size());
        return m_Resources.emplace(&mesh, resources).first->second;
    }

    void Clear() {
        for (auto& entry : m_Resources) {
            MeshGpuResources& resources = entry.second;
            if (resources.ebo != 0) {
                glDeleteBuffers(1, &resources.ebo);
            }
            if (resources.vbo != 0) {
                glDeleteBuffers(1, &resources.vbo);
            }
            if (resources.vao != 0) {
                glDeleteVertexArrays(1, &resources.vao);
            }
        }
        m_Resources.clear();
    }

  private:
    std::unordered_map<const Mesh*, MeshGpuResources> m_Resources;
};

} // namespace RetroRenderer
