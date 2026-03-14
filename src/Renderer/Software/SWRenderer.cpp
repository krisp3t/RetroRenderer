#include "SWRenderer.h"
#include <KrisLogger/Logger.h>
#include <glm/gtx/string_cast.hpp>
#include <array>
#include <utility>
#include <vector>

namespace RetroRenderer {
namespace {
float ComputeLightAmount(const std::vector<LightSnapshot>& lights,
                         const glm::vec3& worldPos,
                         const glm::vec3& normal,
                         const Config& cfg) {
    if (lights.empty()) {
        const glm::vec3 lightDir = glm::normalize(cfg.environment.lightPosition - worldPos);
        return std::max(glm::dot(normal, lightDir), 0.0f);
    }

    float accumulated = 0.0f;
    for (const LightSnapshot& light : lights) {
        if (light.type != LightType::POINT) {
            continue;
        }
        const glm::vec3 lightVector = light.position - worldPos;
        const float lightLengthSq = glm::dot(lightVector, lightVector);
        if (lightLengthSq <= 1e-6f) {
            continue;
        }
        const glm::vec3 lightDir = lightVector * glm::inversesqrt(lightLengthSq);
        accumulated += std::max(glm::dot(normal, lightDir), 0.0f) * std::max(light.intensity, 0.0f);
    }
    return std::clamp(accumulated, 0.0f, 1.0f);
}

glm::vec3 ComputeTriangleLightingNormal(const glm::vec3& worldPos0,
                                        const glm::vec3& worldPos1,
                                        const glm::vec3& worldPos2,
                                        const glm::vec3& normal0,
                                        const glm::vec3& normal1,
                                        const glm::vec3& normal2,
                                        const Config& cfg) {
    if (cfg.retro.flatFaceLighting) {
        const glm::vec3 edge0 = worldPos1 - worldPos0;
        const glm::vec3 edge1 = worldPos2 - worldPos0;
        const glm::vec3 faceNormal = glm::cross(edge0, edge1);
        const float faceNormalLengthSq = glm::dot(faceNormal, faceNormal);
        if (faceNormalLengthSq > 1e-8f) {
            return faceNormal * glm::inversesqrt(faceNormalLengthSq);
        }
    }

    return glm::normalize(normal0 + normal1 + normal2);
}

struct ClipPlane {
    glm::vec3 normal;
    float d;
};

constexpr size_t kMaxClippedPolygonVertices = 12;

struct ClippedPolygon {
    std::array<RasterVertex, kMaxClippedPolygonVertices> vertices{};
    size_t count = 0;
};

float PlaneDistance(const ClipPlane& plane, const RasterVertex& v) {
    return glm::dot(plane.normal, v.position) + plane.d;
}

RasterVertex LerpVertex(const RasterVertex& a, const RasterVertex& b, float t) {
    RasterVertex out;
    out.position = glm::mix(a.position, b.position, t);
    out.texCoords = glm::mix(a.texCoords, b.texCoords, t);
    out.color = glm::mix(a.color, b.color, t);
    out.clipW = glm::mix(a.clipW, b.clipW, t);
    return out;
}

bool PushVertex(ClippedPolygon& polygon, const RasterVertex& vertex) {
    if (polygon.count >= polygon.vertices.size()) {
        return false;
    }
    polygon.vertices[polygon.count++] = vertex;
    return true;
}

bool IsVertexInsideNdc(const RasterVertex& vertex) {
    const glm::vec3& p = vertex.position;
    return p.x >= -1.0f && p.x <= 1.0f &&
           p.y >= -1.0f && p.y <= 1.0f &&
           p.z >= -1.0f && p.z <= 1.0f;
}

bool IsTriangleFullyInsideNdc(const std::array<RasterVertex, 3>& vertices) {
    return IsVertexInsideNdc(vertices[0]) &&
           IsVertexInsideNdc(vertices[1]) &&
           IsVertexInsideNdc(vertices[2]);
}

bool IsTriangleTriviallyRejected(const std::array<RasterVertex, 3>& vertices) {
    const auto allOutside = [&vertices](auto predicate) {
        return predicate(vertices[0].position) &&
               predicate(vertices[1].position) &&
               predicate(vertices[2].position);
    };

    return allOutside([](const glm::vec3& p) { return p.x < -1.0f; }) ||
           allOutside([](const glm::vec3& p) { return p.x > 1.0f; }) ||
           allOutside([](const glm::vec3& p) { return p.y < -1.0f; }) ||
           allOutside([](const glm::vec3& p) { return p.y > 1.0f; }) ||
           allOutside([](const glm::vec3& p) { return p.z < -1.0f; }) ||
           allOutside([](const glm::vec3& p) { return p.z > 1.0f; });
}

void SnapProjectedVertex(RasterVertex& vertex, size_t framebufferWidth, size_t framebufferHeight) {
    if (framebufferWidth == 0 || framebufferHeight == 0) {
        return;
    }

    const glm::vec2 viewport = Rasterizer::NDCToViewport(glm::vec2(vertex.position), framebufferWidth, framebufferHeight);
    const glm::vec2 snappedViewport = glm::round(viewport);
    vertex.position.x = ((snappedViewport.x - 0.5f) / static_cast<float>(framebufferWidth)) * 2.0f - 1.0f;
    vertex.position.y = 1.0f - ((snappedViewport.y - 0.5f) / static_cast<float>(framebufferHeight)) * 2.0f;
}

// TODO: Geometric clipping is in NDC (not homogeneous clip space); good for now but edge cases near w=0 will still be imperfect.
ClippedPolygon ClipPolygonNdc(const std::array<RasterVertex, 3>& inputTriangle) {
    static const ClipPlane kPlanes[] = {
        {{1.0f, 0.0f, 0.0f}, 1.0f}, // x >= -1
        {{-1.0f, 0.0f, 0.0f}, 1.0f}, // x <= 1
        {{0.0f, 1.0f, 0.0f}, 1.0f}, // y >= -1
        {{0.0f, -1.0f, 0.0f}, 1.0f}, // y <= 1
        {{0.0f, 0.0f, 1.0f}, 1.0f}, // z >= -1
        {{0.0f, 0.0f, -1.0f}, 1.0f}, // z <= 1
    };

    ClippedPolygon poly{};
    poly.vertices[0] = inputTriangle[0];
    poly.vertices[1] = inputTriangle[1];
    poly.vertices[2] = inputTriangle[2];
    poly.count = 3;

    ClippedPolygon output{};
    for (const auto& plane : kPlanes) {
        if (poly.count == 0) {
            break;
        }
        output.count = 0;
        RasterVertex prev = poly.vertices[poly.count - 1];
        float prevDist = PlaneDistance(plane, prev);
        bool prevInside = prevDist >= 0.0f;

        for (size_t i = 0; i < poly.count; i++) {
            const RasterVertex& curr = poly.vertices[i];
            float currDist = PlaneDistance(plane, curr);
            bool currInside = currDist >= 0.0f;

            if (currInside != prevInside) {
                float t = prevDist / (prevDist - currDist);
                if (!PushVertex(output, LerpVertex(prev, curr, t))) {
                    output.count = 0;
                    return output;
                }
            }
            if (currInside) {
                if (!PushVertex(output, curr)) {
                    output.count = 0;
                    return output;
                }
            }
            prev = curr;
            prevDist = currDist;
            prevInside = currInside;
        }
        std::swap(poly, output);
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

void SWRenderer::SetSceneLights(const std::vector<LightSnapshot>& lights) {
    m_FrameLights = lights;
}

void SWRenderer::SetFrameConfig(const Config& config) {
    m_FrameConfigSnapshot = config;
}

void SWRenderer::SetFallbackTexture(const Texture* texture) {
    p_FrameFallbackTexture = texture;
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

        const auto& cfg = m_FrameConfigSnapshot;
        std::vector<glm::vec4> clipPositions(mesh.m_Vertices.size());
        std::vector<glm::vec3> transformedNormals(mesh.m_Vertices.size());
        std::vector<glm::vec3> worldPositions(mesh.m_Vertices.size());
        for (size_t vertexIndex = 0; vertexIndex < mesh.m_Vertices.size(); vertexIndex++) {
            const Vertex& sourceVertex = mesh.m_Vertices[vertexIndex];
            clipPositions[vertexIndex] = mvp * sourceVertex.position;
            transformedNormals[vertexIndex] = glm::normalize(glm::vec3(n * glm::vec4(sourceVertex.normal, 0.0f)));
            worldPositions[vertexIndex] = glm::vec3(modelMat * sourceVertex.position);
        }

        const Texture* diffuseTexture = p_FrameFallbackTexture;
        if (diffuseTexture == nullptr && !mesh.m_Textures.empty()) {
            diffuseTexture = &mesh.m_Textures[0];
        }

        for (unsigned int i = 0; i < mesh.m_numFaces; i++) {
            // Input Assembler
            unsigned int baseIndex = i * 3;
            const unsigned int i0 = mesh.m_Indices[baseIndex];
            const unsigned int i1 = mesh.m_Indices[baseIndex + 1];
            const unsigned int i2 = mesh.m_Indices[baseIndex + 2];
            const glm::vec4& clipPos0 = clipPositions[i0];
            const glm::vec4& clipPos1 = clipPositions[i1];
            const glm::vec4& clipPos2 = clipPositions[i2];
            if (std::abs(clipPos0.w) <= 1e-6f || std::abs(clipPos1.w) <= 1e-6f || std::abs(clipPos2.w) <= 1e-6f) {
                continue;
            }

            std::array<RasterVertex, 3> vertices{};
            const unsigned int indices[3] = {i0, i1, i2};
            const glm::vec4 clipPositionsForTri[3] = {clipPos0, clipPos1, clipPos2};
            for (int v = 0; v < 3; v++) {
                const Vertex& sourceVertex = mesh.m_Vertices[indices[v]];
                vertices[v].position = glm::vec3(clipPositionsForTri[v]) / clipPositionsForTri[v].w;
                vertices[v].clipW = clipPositionsForTri[v].w;
                vertices[v].texCoords = sourceVertex.texCoords;
                vertices[v].color = sourceVertex.color;
            }

            if (cfg.retro.snapVertices) {
                for (auto& vertex : vertices) {
                    SnapProjectedVertex(vertex, m_FrameBuffer->width, m_FrameBuffer->height);
                }
            }

            // TODO: backface cull

            // Flat retro lighting in world space.
            const glm::vec3& worldPos0 = worldPositions[i0];
            const glm::vec3& worldPos1 = worldPositions[i1];
            const glm::vec3& worldPos2 = worldPositions[i2];
            const glm::vec3 worldPos = (worldPos0 + worldPos1 + worldPos2) / 3.0f;
            const glm::vec3 normal = ComputeTriangleLightingNormal(
                worldPos0,
                worldPos1,
                worldPos2,
                transformedNormals[i0],
                transformedNormals[i1],
                transformedNormals[i2],
                cfg);
            const float ndotl = ComputeLightAmount(m_FrameLights, worldPos, normal, cfg);
            if (cfg.cull.rasterClip && IsTriangleTriviallyRejected(vertices)) {
                continue;
            }
            if (cfg.cull.geometricClip) {
                if (IsTriangleFullyInsideNdc(vertices)) {
                    Rasterizer::DrawTriangle(*m_FrameBuffer, *m_DepthBuffer, vertices, cfg, ndotl, diffuseTexture);
                    continue;
                }

                const ClippedPolygon clipped = ClipPolygonNdc(vertices);
                if (clipped.count < 3) {
                    continue;
                }
                for (size_t t = 1; t + 1 < clipped.count; t++) {
                    std::array<RasterVertex, 3> tri = {clipped.vertices[0], clipped.vertices[t], clipped.vertices[t + 1]};
                    Rasterizer::DrawTriangle(*m_FrameBuffer, *m_DepthBuffer, tri, cfg, ndotl, diffuseTexture);
                }
            } else {
                Rasterizer::DrawTriangle(*m_FrameBuffer, *m_DepthBuffer, vertices, cfg, ndotl, diffuseTexture);
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
