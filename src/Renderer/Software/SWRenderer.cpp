#include "SWRenderer.h"
#include <KrisLogger/Logger.h>
#include <glm/gtx/string_cast.hpp>
#include <array>
#include <utility>
#include <vector>

namespace RetroRenderer {
namespace {
glm::vec3 ComputeTriangleLightingNormal(const glm::vec3& worldPos0,
                                        const glm::vec3& worldPos1,
                                        const glm::vec3& worldPos2,
                                        const glm::vec3& normal0,
                                        const glm::vec3& normal1,
                                        const glm::vec3& normal2,
                                        const Config& cfg) {
    if (cfg.retro.flatFaceLighting) {
        const glm::vec3 averageNormal = normal0 + normal1 + normal2;
        const float averageNormalLengthSq = glm::dot(averageNormal, averageNormal);
        if (averageNormalLengthSq > 1e-8f) {
            const glm::vec3 normalizedAverage = averageNormal * glm::inversesqrt(averageNormalLengthSq);
            const float alignment0 = glm::dot(normalizedAverage, normal0);
            const float alignment1 = glm::dot(normalizedAverage, normal1);
            const float alignment2 = glm::dot(normalizedAverage, normal2);
            if (alignment0 > 0.999f && alignment1 > 0.999f && alignment2 > 0.999f) {
                return normalizedAverage;
            }
        }

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
    glm::vec4 equation;
};

constexpr size_t kMaxClippedPolygonVertices = 12;

struct ClipVertex {
    glm::vec4 clipPosition = glm::vec4(0.0f);
    glm::vec3 worldPosition = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 texCoords = glm::vec2(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
};

struct ClippedPolygon {
    std::array<ClipVertex, kMaxClippedPolygonVertices> vertices{};
    size_t count = 0;
};

float PlaneDistance(const ClipPlane& plane, const ClipVertex& v) {
    return glm::dot(plane.equation, v.clipPosition);
}

ClipVertex LerpVertex(const ClipVertex& a, const ClipVertex& b, float t) {
    ClipVertex out;
    out.clipPosition = glm::mix(a.clipPosition, b.clipPosition, t);
    out.worldPosition = glm::mix(a.worldPosition, b.worldPosition, t);
    out.normal = glm::mix(a.normal, b.normal, t);
    out.texCoords = glm::mix(a.texCoords, b.texCoords, t);
    out.color = glm::mix(a.color, b.color, t);
    return out;
}

bool PushVertex(ClippedPolygon& polygon, const ClipVertex& vertex) {
    if (polygon.count >= polygon.vertices.size()) {
        return false;
    }
    polygon.vertices[polygon.count++] = vertex;
    return true;
}

bool IsVertexInsideClipSpace(const ClipVertex& vertex) {
    const glm::vec4& p = vertex.clipPosition;
    return p.x >= -p.w && p.x <= p.w &&
           p.y >= -p.w && p.y <= p.w &&
           p.z >= -p.w && p.z <= p.w;
}

bool IsTriangleFullyInsideClipSpace(const std::array<ClipVertex, 3>& vertices) {
    return IsVertexInsideClipSpace(vertices[0]) &&
           IsVertexInsideClipSpace(vertices[1]) &&
           IsVertexInsideClipSpace(vertices[2]);
}

bool IsTriangleTriviallyRejected(const std::array<ClipVertex, 3>& vertices) {
    const auto allOutside = [&vertices](auto predicate) {
        return predicate(vertices[0].clipPosition) &&
               predicate(vertices[1].clipPosition) &&
               predicate(vertices[2].clipPosition);
    };

    return allOutside([](const glm::vec4& p) { return p.x < -p.w; }) ||
           allOutside([](const glm::vec4& p) { return p.x > p.w; }) ||
           allOutside([](const glm::vec4& p) { return p.y < -p.w; }) ||
           allOutside([](const glm::vec4& p) { return p.y > p.w; }) ||
           allOutside([](const glm::vec4& p) { return p.z < -p.w; }) ||
           allOutside([](const glm::vec4& p) { return p.z > p.w; });
}

bool TryMakeRasterVertex(const ClipVertex& clipVertex, RasterVertex& outVertex) {
    if (std::abs(clipVertex.clipPosition.w) <= 1e-6f) {
        return false;
    }

    const glm::vec3 ndcPosition = glm::vec3(clipVertex.clipPosition) / clipVertex.clipPosition.w;
    outVertex.position = ndcPosition;
    outVertex.cullPosition = ndcPosition;
    outVertex.worldPosition = clipVertex.worldPosition;
    outVertex.normal = clipVertex.normal;
    outVertex.clipW = clipVertex.clipPosition.w;
    outVertex.texCoords = clipVertex.texCoords;
    outVertex.color = clipVertex.color;
    return true;
}

bool TryMakeRasterTriangle(const std::array<ClipVertex, 3>& clipVertices, std::array<RasterVertex, 3>& outVertices) {
    for (size_t i = 0; i < clipVertices.size(); i++) {
        if (!TryMakeRasterVertex(clipVertices[i], outVertices[i])) {
            return false;
        }
    }
    return true;
}

void SnapProjectedVertex(RasterVertex& vertex, size_t framebufferWidth, size_t framebufferHeight, float snapStep) {
    if (framebufferWidth == 0 || framebufferHeight == 0) {
        return;
    }

    const float safeSnapStep = std::max(snapStep, 0.001f);
    const glm::vec2 viewport = Rasterizer::NDCToViewport(glm::vec2(vertex.position), framebufferWidth, framebufferHeight);
    const glm::vec2 snappedViewport = glm::round(viewport / safeSnapStep) * safeSnapStep;
    vertex.position.x = ((snappedViewport.x - 0.5f) / static_cast<float>(framebufferWidth)) * 2.0f - 1.0f;
    vertex.position.y = 1.0f - ((snappedViewport.y - 0.5f) / static_cast<float>(framebufferHeight)) * 2.0f;
}

bool ShouldDeferPs1Triangles(const Config& cfg) {
    return cfg.software.rasterizer.polygonMode == Config::RasterizationPolygonMode::FILL &&
           cfg.retro.usePs1ShadingModel &&
           cfg.retro.enablePs1SemiTransparency;
}

float ComputeDeferredTriangleSortKey(const std::array<RasterVertex, 3>& vertices, const glm::vec3& cameraPosition) {
    const glm::vec3 centroid = (vertices[0].worldPosition + vertices[1].worldPosition + vertices[2].worldPosition) / 3.0f;
    const glm::vec3 delta = centroid - cameraPosition;
    return glm::dot(delta, delta);
}

ClippedPolygon ClipPolygonClipSpace(const std::array<ClipVertex, 3>& inputTriangle) {
    static const ClipPlane kPlanes[] = {
        {{1.0f, 0.0f, 0.0f, 1.0f}},   // x >= -w
        {{-1.0f, 0.0f, 0.0f, 1.0f}},  // x <= w
        {{0.0f, 1.0f, 0.0f, 1.0f}},   // y >= -w
        {{0.0f, -1.0f, 0.0f, 1.0f}},  // y <= w
        {{0.0f, 0.0f, 1.0f, 1.0f}},   // z >= -w
        {{0.0f, 0.0f, -1.0f, 1.0f}},  // z <= w
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
        ClipVertex prev = poly.vertices[poly.count - 1];
        float prevDist = PlaneDistance(plane, prev);
        bool prevInside = prevDist >= 0.0f;

        for (size_t i = 0; i < poly.count; i++) {
            const ClipVertex& curr = poly.vertices[i];
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

void SWRenderer::SetMaterialState(const SoftwareMaterialState& materialState) {
    m_FrameMaterialState = materialState;
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
        const bool deferPs1Triangles = ShouldDeferPs1Triangles(cfg);
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
        if (deferPs1Triangles) {
            m_DeferredPs1Triangles.reserve(m_DeferredPs1Triangles.size() + mesh.m_numFaces);
        }

        const auto submitTriangle = [&](const std::array<RasterVertex, 3>& rasterVertices) {
            if (deferPs1Triangles) {
                DeferredTriangle deferredTriangle{};
                deferredTriangle.vertices = rasterVertices;
                deferredTriangle.texture = diffuseTexture;
                deferredTriangle.sortKey = ComputeDeferredTriangleSortKey(rasterVertices, p_Camera->m_Position);
                m_DeferredPs1Triangles.push_back(deferredTriangle);
                return;
            }

            std::array<RasterVertex, 3> drawVertices = rasterVertices;
            Rasterizer::DrawTriangle(
                *m_FrameBuffer,
                *m_DepthBuffer,
                drawVertices,
                cfg,
                m_FrameLights,
                m_FrameMaterialState,
                p_Camera->m_Position,
                diffuseTexture);
        };

        for (unsigned int i = 0; i < mesh.m_numFaces; i++) {
            // Input Assembler
            unsigned int baseIndex = i * 3;
            const unsigned int i0 = mesh.m_Indices[baseIndex];
            const unsigned int i1 = mesh.m_Indices[baseIndex + 1];
            const unsigned int i2 = mesh.m_Indices[baseIndex + 2];
            const glm::vec4& clipPos0 = clipPositions[i0];
            const glm::vec4& clipPos1 = clipPositions[i1];
            const glm::vec4& clipPos2 = clipPositions[i2];
            const unsigned int indices[3] = {i0, i1, i2};

            const glm::vec3& worldPos0 = worldPositions[i0];
            const glm::vec3& worldPos1 = worldPositions[i1];
            const glm::vec3& worldPos2 = worldPositions[i2];
            const glm::vec3 triangleNormal = ComputeTriangleLightingNormal(
                worldPos0,
                worldPos1,
                worldPos2,
                transformedNormals[i0],
                transformedNormals[i1],
                transformedNormals[i2],
                cfg);

            std::array<ClipVertex, 3> clipVertices{};
            const glm::vec4 clipPositionsForTri[3] = {clipPos0, clipPos1, clipPos2};
            for (int v = 0; v < 3; v++) {
                const unsigned int vertexIndex = indices[v];
                const Vertex& sourceVertex = mesh.m_Vertices[vertexIndex];
                clipVertices[v].clipPosition = clipPositionsForTri[v];
                clipVertices[v].worldPosition = worldPositions[vertexIndex];
                clipVertices[v].normal = cfg.retro.flatFaceLighting ? triangleNormal : transformedNormals[vertexIndex];
                clipVertices[v].texCoords = sourceVertex.texCoords;
                clipVertices[v].color = sourceVertex.color;
            }

            if (cfg.cull.rasterClip && IsTriangleTriviallyRejected(clipVertices)) {
                continue;
            }
            if (cfg.cull.geometricClip) {
                std::array<RasterVertex, 3> rasterVertices{};
                if (IsTriangleFullyInsideClipSpace(clipVertices)) {
                    if (!TryMakeRasterTriangle(clipVertices, rasterVertices)) {
                        continue;
                    }
                    if (cfg.retro.snapVertices) {
                        for (auto& vertex : rasterVertices) {
                            SnapProjectedVertex(vertex, m_FrameBuffer->width, m_FrameBuffer->height, cfg.retro.vertexSnapStep);
                        }
                    }
                    submitTriangle(rasterVertices);
                    continue;
                }

                const ClippedPolygon clipped = ClipPolygonClipSpace(clipVertices);
                if (clipped.count < 3) {
                    continue;
                }
                for (size_t t = 1; t + 1 < clipped.count; t++) {
                    const std::array<ClipVertex, 3> clippedTriangle = {
                        clipped.vertices[0],
                        clipped.vertices[t],
                        clipped.vertices[t + 1]};
                    if (!TryMakeRasterTriangle(clippedTriangle, rasterVertices)) {
                        continue;
                    }
                    if (cfg.retro.snapVertices) {
                        for (auto& vertex : rasterVertices) {
                            SnapProjectedVertex(vertex, m_FrameBuffer->width, m_FrameBuffer->height, cfg.retro.vertexSnapStep);
                        }
                    }
                    submitTriangle(rasterVertices);
                }
            } else {
                std::array<RasterVertex, 3> rasterVertices{};
                if (!TryMakeRasterTriangle(clipVertices, rasterVertices)) {
                    continue;
                }
                if (cfg.retro.snapVertices) {
                    for (auto& vertex : rasterVertices) {
                        SnapProjectedVertex(vertex, m_FrameBuffer->width, m_FrameBuffer->height, cfg.retro.vertexSnapStep);
                    }
                }
                submitTriangle(rasterVertices);
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
    m_DeferredPs1Triangles.clear();
}

GLuint SWRenderer::EndFrame() {
    if (!m_DeferredPs1Triangles.empty()) {
        std::stable_sort(
            m_DeferredPs1Triangles.begin(),
            m_DeferredPs1Triangles.end(),
            [](const DeferredTriangle& lhs, const DeferredTriangle& rhs) {
                return lhs.sortKey > rhs.sortKey;
            });

        for (const DeferredTriangle& deferredTriangle : m_DeferredPs1Triangles) {
            std::array<RasterVertex, 3> drawVertices = deferredTriangle.vertices;
            Rasterizer::DrawTriangle(
                *m_FrameBuffer,
                *m_DepthBuffer,
                drawVertices,
                m_FrameConfigSnapshot,
                m_FrameLights,
                m_FrameMaterialState,
                p_Camera->m_Position,
                deferredTriangle.texture);
        }
        m_DeferredPs1Triangles.clear();
    }
    return 0;
}

void SWRenderer::DrawSkybox() {
}

const Buffer<Pixel>& SWRenderer::GetFrameBuffer() const {
    assert(m_FrameBuffer != nullptr && "No render target set. Did you call SWRenderer::Init()?");
    return *m_FrameBuffer;
}
} // namespace RetroRenderer
