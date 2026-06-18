#pragma once

#include "../Scene/Mesh.h"
#include "../Scene/Vertex.h"
#include "FrameSnapshot.h"
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
        return GetOrCreate(&mesh, mesh.GetVertices(), mesh.GetIndices());
    }

    const MeshGpuResources& GetOrCreate(const RenderMeshSnapshot& mesh) {
        return GetOrCreate(&mesh, mesh.vertices, mesh.indices);
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
    const MeshGpuResources& GetOrCreate(const void* key,
                                        const std::vector<Vertex>& vertices,
                                        const std::vector<unsigned int>& indices) {
        auto it = m_Resources.find(key);
        if (it != m_Resources.end()) {
            return it->second;
        }

        MeshGpuResources resources{};
        resources.indexCount = static_cast<GLsizei>(indices.size());

        glGenVertexArrays(1, &resources.vao);
        glGenBuffers(1, &resources.vbo);
        glGenBuffers(1, &resources.ebo);

        glBindVertexArray(resources.vao);

        glBindBuffer(GL_ARRAY_BUFFER, resources.vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
            vertices.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
            indices.data(),
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

        LOGD("Created GL mesh resources: %zu verts, %zu indices", vertices.size(), indices.size());
        return m_Resources.emplace(key, resources).first->second;
    }

    std::unordered_map<const void*, MeshGpuResources> m_Resources;
};

} // namespace RetroRenderer
