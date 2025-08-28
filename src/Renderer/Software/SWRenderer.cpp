#include <KrisLogger/Logger.h>
#include "SWRenderer.h"
#include "../../Engine.h"
#include <glm/gtx/string_cast.hpp>

namespace RetroRenderer
{
    bool SWRenderer::Init(GLuint fbTex, int w, int h)
    {
        try
        {
            m_FrameBuffer = new Buffer<uint32_t>(w, h);
        }
        catch (const std::bad_alloc &)
        {
            LOGE("Failed to create frame buffer");
            return false;
        }
        p_FrameBufferTexture = fbTex;
        m_Rasterizer = std::make_unique<Rasterizer>();
        return true;
    }

    void SWRenderer::Resize(GLuint newFbTex, int w, int h)
    {
        p_FrameBufferTexture = newFbTex;
        m_FrameBuffer = new Buffer<uint32_t>(w, h);
    }

    void SWRenderer::Destroy()
    {
        delete m_FrameBuffer;
    }

    void SWRenderer::SetActiveCamera(const Camera &camera)
    {
        p_Camera = const_cast<Camera *>(&camera);
    }

    /**
     * @brief Draw a model on the frame buffer. Must have triangulated meshes!
     * @param mesh
     */
    void SWRenderer::DrawTriangularMesh(const Model *model)
    {
        assert(p_Camera != nullptr && "No active camera set. Did you call SWRenderer::SetActiveCamera()?");
        assert(m_FrameBuffer != nullptr && "No render target set. Did you call SWRenderer::Init()?");
        assert(model != nullptr && "Tried to draw null model");

        const glm::mat4 &modelMat = model->GetWorldTransform();
        const glm::mat4 &viewMat = p_Camera->m_ViewMat;
        const glm::mat4 &projMat = p_Camera->m_ProjMat;
        const glm::mat4 mv = viewMat * modelMat;
        const glm::mat4 mvp = projMat * mv;
        const glm::mat4 n = glm::transpose(glm::inverse(modelMat));

        /*
		LOGD("Drawing model: %s", model.GetName().c_str());
		LOGD("Model matrix: %s", glm::to_string(modelMat).c_str());
		LOGD("View matrix: %s", glm::to_string(viewMat).c_str());
		LOGD("Projection matrix: %s", glm::to_string(projMat).c_str());
		LOGD("Model-View matrix: %s", glm::to_string(mv).c_str());
		LOGD("Model-View-Projection matrix: %s", glm::to_string(mvp).c_str());
		LOGD("Normal matrix: %s", glm::to_string(n).c_str()); 
        */

        for (const Mesh &mesh: model->m_Meshes)
        {
            assert(mesh.m_Indices.size() % 3 == 0 &&
                   mesh.m_Indices.size() == mesh.m_numFaces * 3 &&
                   "Mesh is not triangulated");

            for (unsigned int i = 0; i < mesh.m_numFaces; i++)
            {
                // Input Assembler
                auto &v0 = mesh.m_Vertices[mesh.m_Indices[i]];
                auto &v1 = mesh.m_Vertices[mesh.m_Indices[i + 1]];
                auto &v2 = mesh.m_Vertices[mesh.m_Indices[i + 2]];
                std::array<Vertex, 3> vertices = {v0, v1, v2};

                // TODO: backface cull

                // Vertex Shader
                for (auto &vertex: vertices)
                {
                    vertex.position = mvp * vertex.position;
                    vertex.normal = glm::normalize(glm::vec3(n * glm::vec4(vertex.normal, 0.0f)));
                }

                // Perspective division
                for (auto &vertex: vertices)
                {
                    vertex.position /= vertex.position.w;
                }

                // Rasterizer
                const auto &cfg = Engine::Get().GetConfig()->software.rasterizer;
                m_Rasterizer->DrawTriangle(*m_FrameBuffer, vertices, cfg);

                // Stats
                // p_stats_->renderedVerts += mesh->m_numVertices;
                // p_stats_->renderedTris += mesh->m_numFaces;
            }
        }
    }

    void SWRenderer::BeforeFrame(const Color &clearColor)
    {
        auto c = clearColor.ToImVec4();
        glBindFramebuffer(GL_FRAMEBUFFER, p_FrameBufferTexture);
		glClearColor(c.x, c.y, c.z, c.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_FrameBuffer->Clear(clearColor.ToRGBA());
    }

    GLuint SWRenderer::EndFrame()
    {
        glBindTexture(GL_TEXTURE_2D, p_FrameBufferTexture);
        glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                0,
                0,
                m_FrameBuffer->width,
                m_FrameBuffer->height,
                GL_RGBA,
				GL_UNSIGNED_INT_24_8,
                m_FrameBuffer->data
        );
        glBindTexture(GL_TEXTURE_2D, 0);
        return p_FrameBufferTexture;
    }

    void SWRenderer::DrawSkybox()
    {
    }
}
