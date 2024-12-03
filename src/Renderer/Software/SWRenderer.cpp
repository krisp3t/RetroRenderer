#include "SWRenderer.h"
#include "../../Base/Logger.h"

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

    void SWRenderer::DrawFrame(Scene &scene)
    {
        //p_FrameBuffer->Clear(0xFFFF00FF);
        for (int i = 0; i < p_FrameBuffer->height; i++)
        {
            for (int j = 0; j < p_FrameBuffer->width; j++)
            {
                if (j % 2 == 0)
                    p_FrameBuffer->Set(j, i, 0xFFFFFFFF);
                else
                    p_FrameBuffer->Set(j, i, 0xFF000000);
            }
        }

        // TODO: build render queue
        /*
        for (auto& mesh : scene.m_Meshes)
        {
            for (int i = 0; i < mesh.m_Indices.size(); i += 3)
            {
                auto v0 = mesh.m_Vertices[mesh.m_Indices[i]];
                auto v1 = mesh.m_Vertices[mesh.m_Indices[i + 1]];
                auto v2 = mesh.m_Vertices[mesh.m_Indices[i + 2]];

                DrawTriangle(v0, v1, v2);
            }
        }
        */
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
            auto v0 = mesh.m_Vertices[mesh.m_Indices[i]];
            auto v1 = mesh.m_Vertices[mesh.m_Indices[i + 1]];
            auto v2 = mesh.m_Vertices[mesh.m_Indices[i + 2]];

            // Vertex Shader

            // Rasterizer
            m_Rasterizer->DrawTriangle(*p_FrameBuffer, v0, v1, v2);
        }
    }
}