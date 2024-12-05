#include "SWRenderer.h"
#include "../../Base/Logger.h"
#include "../../Engine.h"

namespace RetroRenderer
{
    bool SWRenderer::Init(int w, int h)
    {
        try
        {
            p_FrameBuffer = new Buffer<Uint32>(w, h);
        }
        catch (const std::bad_alloc&)
        {
            LOGE("Failed to create frame buffer");
            return false;
        }
        m_Rasterizer = std::make_unique<Rasterizer>();
        return true;
    }

    void SWRenderer::Destroy()
    {
    }

    Buffer<Uint32>& SWRenderer::GetRenderTarget()
    {
        assert(p_FrameBuffer != nullptr && "Tried to get null render target. Did you call SWRenderer::Init()?");
        return *p_FrameBuffer;
    }

    void SWRenderer::SetActiveCamera(const Camera &camera)
    {
        p_Camera = const_cast<Camera*>(&camera);
    }

    /**
     * @brief Draw a mesh on the frame buffer. Must be triangulated!
     * @param mesh
     */
    void SWRenderer::DrawTriangularMesh(const Mesh &mesh)
    {
        for (int i = 0; i < mesh.m_Indices.size(); i += 3)
        {
            // Input Assembler
            auto& v0 = mesh.m_Vertices[mesh.m_Indices[i]];
            auto& v1 = mesh.m_Vertices[mesh.m_Indices[i + 1]];
            auto& v2 = mesh.m_Vertices[mesh.m_Indices[i + 2]];
			std::array<Vertex, 3> vertices = { v0, v1, v2 };

            // Vertex Shader

            // Rasterizer
            const auto& cfg = Engine::Get().GetConfig()->rasterizer;
            m_Rasterizer->DrawTriangle(*p_FrameBuffer, vertices, cfg);
        }
    }
}