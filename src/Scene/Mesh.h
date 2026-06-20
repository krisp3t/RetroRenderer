#pragma once
#include "../Renderer/MaterialTypes.h"
#include "Texture.h"
#include "Vertex.h"
#include <vector>

namespace RetroRenderer {

class Mesh {
  public:
    Mesh() = default;
    ~Mesh() = default;
    Mesh(std::vector<Vertex> vertices,
         std::vector<unsigned int> indices,
         std::vector<Texture> textures,
         SceneMaterialHandle materialHandle = kInvalidSceneMaterialHandle);
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    [[nodiscard]] const std::vector<Vertex>& GetVertices() const;
    [[nodiscard]] const std::vector<unsigned int>& GetIndices() const;
    [[nodiscard]] const std::vector<Texture>& GetTextures() const;
    [[nodiscard]] const Texture* GetPrimaryTexture() const;
    [[nodiscard]] SceneMaterialHandle GetMaterialHandle() const;
    [[nodiscard]] unsigned int GetVertexCount() const;
    [[nodiscard]] unsigned int GetFaceCount() const;

    // Per-vertex
    unsigned int m_numVertices = 0;
    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;

    // Per-face
    unsigned int m_numFaces = 0;

    // Per-mesh
    std::vector<Texture> m_Textures;
    SceneMaterialHandle m_MaterialHandle = kInvalidSceneMaterialHandle;
};

} // namespace RetroRenderer
