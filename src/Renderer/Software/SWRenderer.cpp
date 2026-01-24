#include "SWRenderer.h"
#include "../../Engine.h"
#include <KrisLogger/Logger.h>
#include <glm/gtx/string_cast.hpp>

namespace RetroRenderer {
bool SWRenderer::Init(int w, int h) {
    auto fb = std::unique_ptr<Buffer<uint32_t>>(
        new (std::nothrow) Buffer<uint32_t>(w, h)
    );
    if (!fb) {
        LOGE("Failed to create software framebuffer");
        return false;
    }
    m_FrameBuffer = std::move(fb);
    m_Rasterizer = std::make_unique<Rasterizer>();
    return true;
}

bool SWRenderer::Resize(int w, int h) {
    auto newBuffer = std::unique_ptr<Buffer<uint32_t>>(
    new (std::nothrow) Buffer<uint32_t>(w, h)
    );
    if (!newBuffer) {
        LOGE("Failed to resize software framebuffer");
        return false;
    }
    m_FrameBuffer = std::move(newBuffer);
    return true;
}

void SWRenderer::SetActiveCamera(const Camera &camera) {
    p_Camera = const_cast<Camera *>(&camera);
}

/**
 * @brief Draw a model on the frame buffer. Must have triangulated meshes!
 * @param mesh
 */
void SWRenderer::DrawTriangularMesh(const Model *model) {
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

    for (const Mesh &mesh : model->m_Meshes) {
        assert(mesh.m_Indices.size() % 3 == 0 && mesh.m_Indices.size() == mesh.m_numFaces * 3 &&
               "Mesh is not triangulated");

        for (unsigned int i = 0; i < mesh.m_numFaces; i++) {
            // Input Assembler
            unsigned int baseIndex = i * 3;
            auto &v0 = mesh.m_Vertices[mesh.m_Indices[baseIndex]];
            auto &v1 = mesh.m_Vertices[mesh.m_Indices[baseIndex + 1]];
            auto &v2 = mesh.m_Vertices[mesh.m_Indices[baseIndex + 2]];
            std::array<Vertex, 3> vertices = {v0, v1, v2};

            // TODO: backface cull

            // Vertex Shader
            for (auto &vertex : vertices) {
                vertex.position = mvp * vertex.position;
                vertex.normal = glm::normalize(glm::vec3(n * glm::vec4(vertex.normal, 0.0f)));
            }

            // Perspective division
            for (auto &vertex : vertices) {
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

void SWRenderer::BeforeFrame(const Color &clearColor) {
    m_FrameBuffer->Clear(clearColor.ToRGBA());
}

GLuint SWRenderer::EndFrame() {
    return 0;
}

void SWRenderer::DrawSkybox() {
}

const Buffer<uint32_t> &SWRenderer::GetFrameBuffer() const {
    assert(m_FrameBuffer != nullptr && "No render target set. Did you call SWRenderer::Init()?");
    return *m_FrameBuffer;
}
} // namespace RetroRenderer
