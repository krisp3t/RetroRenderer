#include "SWRenderer.h"
#include <KrisLogger/Logger.h>
#include <glm/gtx/string_cast.hpp>
#include <vector>

namespace RetroRenderer {
namespace {
struct ClipPlane {
    glm::vec3 normal;
    float d;
};

float PlaneDistance(const ClipPlane& plane, const Vertex& v) {
    return glm::dot(plane.normal, glm::vec3(v.position)) + plane.d;
}

Vertex LerpVertex(const Vertex& a, const Vertex& b, float t) {
    Vertex out;
    out.position = glm::mix(a.position, b.position, t);
    out.normal = glm::normalize(glm::mix(a.normal, b.normal, t));
    out.texCoords = glm::mix(a.texCoords, b.texCoords, t);
    out.color = glm::mix(a.color, b.color, t);
    return out;
}

// TODO: Geometric clipping is in NDC (not homogeneous clip space); good for now but edge cases near w=0 will still be imperfect.
std::vector<Vertex> ClipPolygonNdc(const std::vector<Vertex>& input) {
    static const ClipPlane kPlanes[] = {
        {{1.0f, 0.0f, 0.0f}, 1.0f}, // x >= -1
        {{-1.0f, 0.0f, 0.0f}, 1.0f}, // x <= 1
        {{0.0f, 1.0f, 0.0f}, 1.0f}, // y >= -1
        {{0.0f, -1.0f, 0.0f}, 1.0f}, // y <= 1
        {{0.0f, 0.0f, 1.0f}, 1.0f}, // z >= -1
        {{0.0f, 0.0f, -1.0f}, 1.0f}, // z <= 1
    };

    std::vector<Vertex> poly = input;
    std::vector<Vertex> output;
    for (const auto& plane : kPlanes) {
        if (poly.empty()) {
            break;
        }
        output.clear();
        Vertex prev = poly.back();
        float prevDist = PlaneDistance(plane, prev);
        bool prevInside = prevDist >= 0.0f;

        for (const auto& curr : poly) {
            float currDist = PlaneDistance(plane, curr);
            bool currInside = currDist >= 0.0f;

            if (currInside != prevInside) {
                float t = prevDist / (prevDist - currDist);
                output.push_back(LerpVertex(prev, curr, t));
            }
            if (currInside) {
                output.push_back(curr);
            }
            prev = curr;
            prevDist = currDist;
            prevInside = currInside;
        }
        poly = output;
    }
    return poly;
}
} // namespace
bool SWRenderer::Init(int w, int h) {
    auto fb = std::unique_ptr<Buffer<Pixel>>(new(std::nothrow) Buffer<Pixel>(w, h));
    if (!fb) {
        LOGE("Failed to create software framebuffer");
        return false;
    }
    m_FrameBuffer = std::move(fb);
    m_DepthBuffer = std::make_unique<Buffer<float>>(w, h);
    m_Rasterizer = std::make_unique<Rasterizer>();
    return true;
}

bool SWRenderer::Resize(int w, int h) {
    auto newBuffer = std::unique_ptr<Buffer<Pixel>>(new(std::nothrow) Buffer<Pixel>(w, h));
    if (!newBuffer) {
        LOGE("Failed to resize software framebuffer");
        return false;
    }
    m_FrameBuffer = std::move(newBuffer);
    m_DepthBuffer = std::make_unique<Buffer<float>>(w, h);
    return true;
}

void SWRenderer::SetActiveCamera(const Camera& camera) {
    p_Camera = const_cast<Camera*>(&camera);
}

void SWRenderer::SetFrameConfig(const Config& config) {
    m_FrameConfigSnapshot = config;
}

/**
 * @brief Draw a model on the framebuffer. Must have triangulated meshes!
 * @param mesh
 */
void SWRenderer::DrawTriangularMesh(const Model* model) {
    assert(p_Camera != nullptr && "No active camera set. Did you call SWRenderer::SetActiveCamera()?");
    assert(m_FrameBuffer != nullptr && "No render target set. Did you call SWRenderer::Init()?");
    assert(m_DepthBuffer != nullptr && "No depth buffer set. Did you call SWRenderer::Init()?");
    assert(model != nullptr && "Tried to draw null model");

    const glm::mat4& modelMat = model->GetWorldTransform();
    const glm::mat4& viewMat = p_Camera->m_ViewMat;
    const glm::mat4& projMat = p_Camera->m_ProjMat;
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

    for (const Mesh& mesh : model->m_Meshes) {
        assert(mesh.m_Indices.size() % 3 == 0 && mesh.m_Indices.size() == mesh.m_numFaces * 3 &&
            "Mesh is not triangulated");

        for (unsigned int i = 0; i < mesh.m_numFaces; i++) {
            // Input Assembler
            unsigned int baseIndex = i * 3;
            auto& v0 = mesh.m_Vertices[mesh.m_Indices[baseIndex]];
            auto& v1 = mesh.m_Vertices[mesh.m_Indices[baseIndex + 1]];
            auto& v2 = mesh.m_Vertices[mesh.m_Indices[baseIndex + 2]];
            std::array<Vertex, 3> vertices = {v0, v1, v2};

            // TODO: backface cull

            // Vertex Shader
            for (auto& vertex : vertices) {
                vertex.position = mvp * vertex.position;
                vertex.normal = glm::normalize(glm::vec3(n * glm::vec4(vertex.normal, 0.0f)));
            }

            // Flat lighting (Lambert) in world space.
            const glm::vec3 worldPos0 = glm::vec3(modelMat * v0.position);
            const glm::vec3 worldPos1 = glm::vec3(modelMat * v1.position);
            const glm::vec3 worldPos2 = glm::vec3(modelMat * v2.position);
            const glm::vec3 worldPos = (worldPos0 + worldPos1 + worldPos2) / 3.0f;
            const glm::vec3 normal = glm::normalize(v0.normal + v1.normal + v2.normal);
            const glm::vec3 lightDir = glm::normalize(m_FrameConfigSnapshot.environment.lightPosition - worldPos);
            const float ndotl = std::max(glm::dot(normal, lightDir), 0.0f);
            const Color lit(Color::FloatTag{}, ndotl, ndotl, ndotl, 1.0f);

            // Perspective division
            for (auto& vertex : vertices) {
                vertex.position /= vertex.position.w;
            }

            // Rasterizer
            const auto& cfg = m_FrameConfigSnapshot;
            // Early-reject in NDC
            if (cfg.cull.rasterClip) {
                bool allOutside = true;
                for (const auto& v : vertices) {
                    if (v.position.x >= -1.0f && v.position.x <= 1.0f &&
                        v.position.y >= -1.0f && v.position.y <= 1.0f &&
                        v.position.z >= -1.0f && v.position.z <= 1.0f) {
                        allOutside = false;
                        break;
                    }
                }
                if (allOutside) {
                    continue;
                }
            }
            if (cfg.cull.geometricClip) {
                std::vector<Vertex> poly = {vertices[0], vertices[1], vertices[2]};
                std::vector<Vertex> clipped = ClipPolygonNdc(poly);
                if (clipped.size() < 3) {
                    continue;
                }
                for (size_t t = 1; t + 1 < clipped.size(); t++) {
                    std::array<Vertex, 3> tri = {clipped[0], clipped[t], clipped[t + 1]};
                    Rasterizer::DrawTriangle(*m_FrameBuffer, *m_DepthBuffer, tri, cfg, lit.ToPixel());
                }
            } else {
                Rasterizer::DrawTriangle(*m_FrameBuffer, *m_DepthBuffer, vertices, cfg, lit.ToPixel());
            }

            // Stats
            // p_stats_->renderedVerts += mesh->m_numVertices;
            // p_stats_->renderedTris += mesh->m_numFaces;
        }
    }
}

void SWRenderer::BeforeFrame(const Color& clearColor) {
    m_FrameBuffer->Clear(clearColor.ToPixel());
    if (m_DepthBuffer) {
        m_DepthBuffer->Clear(1.0f);
    }
}

GLuint SWRenderer::EndFrame() {
    return 0;
}

void SWRenderer::DrawSkybox() {
}

const Buffer<Pixel>& SWRenderer::GetFrameBuffer() const {
    assert(m_FrameBuffer != nullptr && "No render target set. Did you call SWRenderer::Init()?");
    return *m_FrameBuffer;
}
} // namespace RetroRenderer
