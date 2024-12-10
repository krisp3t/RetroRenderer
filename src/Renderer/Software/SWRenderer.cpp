#include "SWRenderer.h"
#include "../../Base/Logger.h"
#include "../../Engine.h"
#include <glm/gtx/string_cast.hpp>

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
     * @brief Draw a model on the frame buffer. Must have triangulated meshes!
     * @param mesh
     */
    void SWRenderer::DrawTriangularMesh(const Model &model)
    {
		const glm::mat4& modelMat = model.GetTransform();
        const glm::mat4& viewMat = p_Camera->viewMat;
        const glm::mat4& projMat = p_Camera->projMat;
		const glm::mat4 mv = viewMat * modelMat;
		const glm::mat4 mvp = projMat * mv;
		const glm::mat4 n = glm::transpose(glm::inverse(modelMat));

		LOGD("Drawing model: %s", model.GetName().c_str());
		LOGD("Model matrix: %s", glm::to_string(modelMat).c_str());
		LOGD("View matrix: %s", glm::to_string(viewMat).c_str());
		LOGD("Projection matrix: %s", glm::to_string(projMat).c_str());
		LOGD("Model-View matrix: %s", glm::to_string(mv).c_str());
		LOGD("Model-View-Projection matrix: %s", glm::to_string(mvp).c_str());
		LOGD("Normal matrix: %s", glm::to_string(n).c_str());


        for (const Mesh* mesh : model.GetMeshes())
        {
			assert(mesh->m_Indices.size() % 3 == 0 && 
				   mesh->m_Indices.size() == mesh->m_numFaces * 3 &&
                   "Mesh is not triangulated");

			for (int i = 0; i < mesh->m_numFaces; i ++)
			{
				// Input Assembler
				auto& v0 = mesh->m_Vertices[mesh->m_Indices[i]];
				auto& v1 = mesh->m_Vertices[mesh->m_Indices[i + 1]];
				auto& v2 = mesh->m_Vertices[mesh->m_Indices[i + 2]];
				std::array<Vertex, 3> vertices = { v0, v1, v2 };

                // TODO: backface cull
                
				// Vertex Shader
				for (auto& vertex : vertices)
				{
					vertex.position = mvp * vertex.position;
					vertex.normal = glm::normalize(glm::vec3(n * glm::vec4(vertex.normal, 0.0f)));
				}

                // Perspective division
				for (auto& vertex : vertices)
				{
                    vertex.position /= vertex.position.w;
				}

				// Rasterizer
				const auto& cfg = Engine::Get().GetConfig()->rasterizer;
				m_Rasterizer->DrawTriangle(*p_FrameBuffer, vertices, cfg);

				// Stats
				// p_Stats->renderedVerts += mesh->m_numVertices;
				// p_Stats->renderedTris += mesh->m_numFaces;
			}
        }
    }
}