#include "SWRenderer.h"
#include "../GridGizmo.h"
#include <SDL_image.h>
#include <KrisLogger/Logger.h>
#include <glm/gtx/string_cast.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <string>
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
constexpr float kClipEpsilon = 1e-5f;

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

bool IsVertexInsideDepthClipSpace(const ClipVertex& vertex) {
    const glm::vec4& p = vertex.clipPosition;
    return p.z >= -p.w - kClipEpsilon &&
           p.z <= p.w + kClipEpsilon;
}

bool IsTriangleFullyInsideDepthClipSpace(const std::array<ClipVertex, 3>& vertices) {
    return IsVertexInsideDepthClipSpace(vertices[0]) &&
           IsVertexInsideDepthClipSpace(vertices[1]) &&
           IsVertexInsideDepthClipSpace(vertices[2]);
}

bool IsTriangleTriviallyRejectedByDepth(const std::array<ClipVertex, 3>& vertices) {
    const auto allOutside = [&vertices](auto predicate) {
        return predicate(vertices[0].clipPosition) &&
               predicate(vertices[1].clipPosition) &&
               predicate(vertices[2].clipPosition);
    };

    return allOutside([](const glm::vec4& p) { return p.z < -p.w - kClipEpsilon; }) ||
           allOutside([](const glm::vec4& p) { return p.z > p.w + kClipEpsilon; });
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

Pixel GridColorToPixel(const glm::vec3& color) {
    return Pixel{
        static_cast<uint8_t>(std::clamp(std::lround(color.r * 255.0f), 0L, 255L)),
        static_cast<uint8_t>(std::clamp(std::lround(color.g * 255.0f), 0L, 255L)),
        static_cast<uint8_t>(std::clamp(std::lround(color.b * 255.0f), 0L, 255L)),
        255};
}

float QuantizeGridDepth(float z, const Config& cfg) {
    const int bits = cfg.retro.depthPrecisionBits;
    if (bits <= 0) {
        return z;
    }

    const uint32_t levels = (1u << std::min(bits, 24)) - 1u;
    if (levels == 0u) {
        return z;
    }

    const float clampedZ = std::clamp(z, 0.0f, 1.0f);
    const float quantized = std::round(clampedZ * static_cast<float>(levels)) / static_cast<float>(levels);
    if (!cfg.retro.usePs1ShadingModel) {
        return quantized;
    }

    const float bucketEpsilon = 1.0f / (static_cast<float>(levels) * 4096.0f);
    return std::min(1.0f, quantized + clampedZ * bucketEpsilon);
}

bool ClipLineSegmentClipSpace(glm::vec4& a, glm::vec4& b) {
    static const ClipPlane kPlanes[] = {
        {{1.0f, 0.0f, 0.0f, 1.0f}},   // x >= -w
        {{-1.0f, 0.0f, 0.0f, 1.0f}},  // x <= w
        {{0.0f, 1.0f, 0.0f, 1.0f}},   // y >= -w
        {{0.0f, -1.0f, 0.0f, 1.0f}},  // y <= w
        {{0.0f, 0.0f, 1.0f, 1.0f}},   // z >= -w
        {{0.0f, 0.0f, -1.0f, 1.0f}},  // z <= w
    };

    for (const ClipPlane& plane : kPlanes) {
        float distanceA = glm::dot(plane.equation, a);
        float distanceB = glm::dot(plane.equation, b);
        const bool aInside = distanceA >= -kClipEpsilon;
        const bool bInside = distanceB >= -kClipEpsilon;
        if (!aInside && !bInside) {
            return false;
        }
        if (aInside && bInside) {
            continue;
        }

        const float denominator = distanceA - distanceB;
        if (std::abs(denominator) <= 1e-8f) {
            return false;
        }
        const float t = std::clamp(distanceA / denominator, 0.0f, 1.0f);
        const glm::vec4 intersection = glm::mix(a, b, t);
        if (!aInside) {
            a = intersection;
            distanceA = 0.0f;
        } else {
            b = intersection;
            distanceB = 0.0f;
        }
    }
    return true;
}

void DrawDepthTestedLine(Buffer<Pixel>& framebuffer,
                         Buffer<float>& depthBuffer,
                         const glm::vec3& start,
                         const glm::vec3& end,
                         const Config& cfg,
                         Pixel color) {
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float dz = end.z - start.z;
    const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dy)))));

    float x = start.x;
    float y = start.y;
    float z = start.z;
    const float invSteps = 1.0f / static_cast<float>(steps);
    const float xStep = dx * invSteps;
    const float yStep = dy * invSteps;
    const float zStep = dz * invSteps;

    for (int i = 0; i <= steps; i++) {
        const int pixelX = static_cast<int>(std::lround(x));
        const int pixelY = static_cast<int>(std::lround(y));
        if (pixelX >= 0 && pixelX < static_cast<int>(framebuffer.width) &&
            pixelY >= 0 && pixelY < static_cast<int>(framebuffer.height)) {
            const size_t pixelIndex = static_cast<size_t>(pixelY) * framebuffer.width + static_cast<size_t>(pixelX);
            const float depth = QuantizeGridDepth(z, cfg);
            if (!cfg.cull.depthTest || depth < depthBuffer.data[pixelIndex]) {
                framebuffer.data[pixelIndex] = color;
                if (cfg.cull.depthTest) {
                    depthBuffer.data[pixelIndex] = depth;
                }
            }
        }

        x += xStep;
        y += yStep;
        z += zStep;
    }
}

ClippedPolygon ClipPolygonDepthClipSpace(const std::array<ClipVertex, 3>& inputTriangle) {
    static const ClipPlane kPlanes[] = {
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
        bool prevInside = prevDist >= -kClipEpsilon;

        for (size_t i = 0; i < poly.count; i++) {
            const ClipVertex& curr = poly.vertices[i];
            float currDist = PlaneDistance(plane, curr);
            bool currInside = currDist >= -kClipEpsilon;

            if (currInside != prevInside) {
                const float denominator = prevDist - currDist;
                if (std::abs(denominator) <= 1e-8f) {
                    prev = curr;
                    prevDist = currDist;
                    prevInside = currInside;
                    continue;
                }
                const float t = std::clamp(prevDist / denominator, 0.0f, 1.0f);
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

std::string DefaultSkyboxCrossPath() {
#ifdef __ANDROID__
    return "img/skybox-cubemap/Cubemap_Sky_23-512x512.png";
#else
    return "assets/img/skybox-cubemap/Cubemap_Sky_23-512x512.png";
#endif
}

bool LoadSkyboxCrossImage(const std::string& path, int& outFaceSize, std::array<std::vector<Pixel>, 6>& outFaces) {
    const int flags = IMG_INIT_PNG;
    const int initted = IMG_Init(flags);
    if ((initted & flags) != flags) {
        LOGE("Failed to initialize SDL_image for software skybox: %s", IMG_GetError());
        return false;
    }

    SDL_Surface* sourceSurface = IMG_Load(path.c_str());
    if (!sourceSurface) {
        LOGE("Failed to load software skybox %s: %s", path.c_str(), IMG_GetError());
        return false;
    }

    SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(sourceSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(sourceSurface);
    if (!rgbaSurface) {
        return false;
    }

    const int fullWidth = rgbaSurface->w;
    const int fullHeight = rgbaSurface->h;
    const int faceSize = fullWidth / 4;
    if (faceSize <= 0 || fullHeight != faceSize * 3) {
        LOGE("Invalid software skybox cross layout dimensions: %dx%d", fullWidth, fullHeight);
        SDL_FreeSurface(rgbaSurface);
        return false;
    }

    const Pixel* surfacePixels = static_cast<const Pixel*>(rgbaSurface->pixels);
    struct Offset {
        int x;
        int y;
    };
    static constexpr Offset kFaceOffsets[6] = {
        {2, 1}, // +X
        {0, 1}, // -X
        {1, 0}, // +Y
        {1, 2}, // -Y
        {1, 1}, // +Z
        {3, 1}, // -Z
    };

    for (size_t faceIndex = 0; faceIndex < outFaces.size(); faceIndex++) {
        std::vector<Pixel>& facePixels = outFaces[faceIndex];
        facePixels.resize(static_cast<size_t>(faceSize) * static_cast<size_t>(faceSize));

        const int offsetX = kFaceOffsets[faceIndex].x * faceSize;
        const int offsetY = kFaceOffsets[faceIndex].y * faceSize;
        for (int y = 0; y < faceSize; y++) {
            for (int x = 0; x < faceSize; x++) {
                const int sourceX = offsetX + x;
                const int sourceY = offsetY + y;
                facePixels[static_cast<size_t>(y) * static_cast<size_t>(faceSize) + static_cast<size_t>(x)] =
                    surfacePixels[static_cast<size_t>(sourceY) * static_cast<size_t>(fullWidth) + static_cast<size_t>(sourceX)];
            }
        }
    }

    outFaceSize = faceSize;
    SDL_FreeSurface(rgbaSurface);
    return true;
}

Pixel SampleSkyboxFace(const std::vector<Pixel>& facePixels, int faceSize, float u, float v) {
    if (facePixels.empty() || faceSize <= 0) {
        return Pixel{0, 0, 0, 255};
    }

    const float clampedU = std::clamp(u, 0.0f, 1.0f);
    const float clampedV = std::clamp(v, 0.0f, 1.0f);
    const int x = std::clamp(static_cast<int>(std::floor(clampedU * static_cast<float>(faceSize))), 0, faceSize - 1);
    const int y = std::clamp(static_cast<int>(std::floor((1.0f - clampedV) * static_cast<float>(faceSize))), 0, faceSize - 1);
    return facePixels[static_cast<size_t>(y) * static_cast<size_t>(faceSize) + static_cast<size_t>(x)];
}

Pixel SampleSkyboxCubemap(const std::array<std::vector<Pixel>, 6>& faces, int faceSize, const glm::vec3& direction) {
    const glm::vec3 dir = glm::normalize(direction);
    const float absX = std::abs(dir.x);
    const float absY = std::abs(dir.y);
    const float absZ = std::abs(dir.z);

    size_t faceIndex = 0;
    float u = 0.5f;
    float v = 0.5f;
    if (absX >= absY && absX >= absZ) {
        if (dir.x >= 0.0f) {
            faceIndex = 0; // +X
            u = (-dir.z / absX + 1.0f) * 0.5f;
            v = (-dir.y / absX + 1.0f) * 0.5f;
        } else {
            faceIndex = 1; // -X
            u = (dir.z / absX + 1.0f) * 0.5f;
            v = (-dir.y / absX + 1.0f) * 0.5f;
        }
    } else if (absY >= absX && absY >= absZ) {
        if (dir.y >= 0.0f) {
            faceIndex = 2; // +Y
            u = (dir.x / absY + 1.0f) * 0.5f;
            v = (dir.z / absY + 1.0f) * 0.5f;
        } else {
            faceIndex = 3; // -Y
            u = (dir.x / absY + 1.0f) * 0.5f;
            v = (-dir.z / absY + 1.0f) * 0.5f;
        }
    } else {
        if (dir.z >= 0.0f) {
            faceIndex = 4; // +Z
            u = (dir.x / absZ + 1.0f) * 0.5f;
            v = (-dir.y / absZ + 1.0f) * 0.5f;
        } else {
            faceIndex = 5; // -Z
            u = (-dir.x / absZ + 1.0f) * 0.5f;
            v = (-dir.y / absZ + 1.0f) * 0.5f;
        }
    }

    return SampleSkyboxFace(faces[faceIndex], faceSize, u, v);
}

size_t ComputeSkyboxSampleStep(size_t width, size_t height) {
    return std::max<size_t>(1, (std::max(width, height) + 319) / 320);
}

bool NearlyEqual(float a, float b, float epsilon = 1e-4f) {
    return std::abs(a - b) <= epsilon;
}

bool NearlyEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = 1e-4f) {
    return glm::all(glm::lessThanEqual(glm::abs(a - b), glm::vec3(epsilon)));
}

bool SkyboxCacheMatches(const Camera& camera,
                        size_t width,
                        size_t height,
                        CameraType cachedType,
                        const glm::vec3& cachedDirection,
                        float cachedFov,
                        float cachedOrthoSize,
                        size_t cachedWidth,
                        size_t cachedHeight) {
    return cachedWidth == width &&
           cachedHeight == height &&
           cachedType == camera.m_Type &&
           NearlyEqual(cachedDirection, glm::normalize(camera.m_Direction)) &&
           NearlyEqual(cachedFov, camera.m_Fov) &&
           NearlyEqual(cachedOrthoSize, camera.m_OrthoSize);
}

void BuildSkyboxCache(std::vector<Pixel>& outPixels,
                      size_t width,
                      size_t height,
                      const Camera& camera,
                      const std::array<std::vector<Pixel>, 6>& faces,
                      int faceSize) {
    if (width == 0 || height == 0) {
        outPixels.clear();
        return;
    }

    outPixels.resize(width * height);
    Pixel* pixels = outPixels.data();
    const glm::vec3 forward = glm::normalize(camera.m_Direction);
    const glm::vec3 right = glm::normalize(glm::cross(forward, camera.m_Up));
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));
    const float widthF = static_cast<float>(width);
    const float heightF = static_cast<float>(height);
    const float aspect = heightF > 0.0f ? widthF / heightF : 1.0f;
    const size_t sampleStep = ComputeSkyboxSampleStep(width, height);

    const auto fillSkyboxBlock = [&](size_t startX, size_t startY, const Pixel& color) {
        const size_t endX = std::min(startX + sampleStep, width);
        const size_t endY = std::min(startY + sampleStep, height);
        for (size_t fillY = startY; fillY < endY; fillY++) {
            Pixel* row = pixels + fillY * width;
            std::fill_n(row + startX, endX - startX, color);
        }
    };

    if (camera.m_Type == CameraType::PERSPECTIVE) {
        const float tanHalfFov = std::tan(glm::radians(camera.m_Fov) * 0.5f);
        for (size_t y = 0; y < height; y += sampleStep) {
            const size_t sampleY = std::min(y + sampleStep / 2, height - 1);
            const float ndcY = 1.0f - ((static_cast<float>(sampleY) + 0.5f) / heightF) * 2.0f;
            for (size_t x = 0; x < width; x += sampleStep) {
                const size_t sampleX = std::min(x + sampleStep / 2, width - 1);
                const float ndcX = ((static_cast<float>(sampleX) + 0.5f) / widthF) * 2.0f - 1.0f;
                const glm::vec3 rayDir =
                    glm::normalize(forward + right * (ndcX * aspect * tanHalfFov) + up * (ndcY * tanHalfFov));
                fillSkyboxBlock(x, y, SampleSkyboxCubemap(faces, faceSize, rayDir));
            }
        }
        return;
    }

    const float orthoScale = std::max(camera.m_OrthoSize, 0.001f);
    for (size_t y = 0; y < height; y += sampleStep) {
        const size_t sampleY = std::min(y + sampleStep / 2, height - 1);
        const float ndcY = 1.0f - ((static_cast<float>(sampleY) + 0.5f) / heightF) * 2.0f;
        for (size_t x = 0; x < width; x += sampleStep) {
            const size_t sampleX = std::min(x + sampleStep / 2, width - 1);
            const float ndcX = ((static_cast<float>(sampleX) + 0.5f) / widthF) * 2.0f - 1.0f;
            const glm::vec3 rayDir =
                glm::normalize(forward + right * (ndcX * aspect * orthoScale) + up * (ndcY * orthoScale));
            fillSkyboxBlock(x, y, SampleSkyboxCubemap(faces, faceSize, rayDir));
        }
    }
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
    m_HasSkybox = LoadSkyboxCrossImage(DefaultSkyboxCrossPath(), m_SkyboxFaceSize, m_SkyboxFaces);
    if (!m_HasSkybox) {
        LOGW("Failed to load software skybox from %s", DefaultSkyboxCrossPath().c_str());
    }
    m_SkyboxCacheValid = false;
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
    m_SkyboxCachePixels.clear();
    m_SkyboxCacheWidth = 0;
    m_SkyboxCacheHeight = 0;
    m_SkyboxCacheValid = false;
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

            if (cfg.cull.rasterClip && IsTriangleTriviallyRejectedByDepth(clipVertices)) {
                continue;
            }
            if (cfg.cull.geometricClip) {
                std::array<RasterVertex, 3> rasterVertices{};
                if (IsTriangleFullyInsideDepthClipSpace(clipVertices)) {
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

                const ClippedPolygon clipped = ClipPolygonDepthClipSpace(clipVertices);
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
    ApplyOutlinePass();
    return 0;
}

void SWRenderer::ApplyOutlinePass() {
    if (!m_FrameBuffer ||
        !m_DepthBuffer ||
        !m_FrameConfigSnapshot.retro.enableOutline ||
        m_FrameConfigSnapshot.software.rasterizer.polygonMode != Config::RasterizationPolygonMode::FILL) {
        return;
    }

    const size_t width = m_FrameBuffer->width;
    const size_t height = m_FrameBuffer->height;
    if (width == 0 || height == 0) {
        return;
    }

    const int radius = std::clamp(m_FrameConfigSnapshot.retro.outlineThickness, 1, 4);
    const Pixel outlineColor = m_FrameConfigSnapshot.retro.outlineColor.ToPixel();
    const float backgroundDepth = 1.0f - 1e-4f;
    std::vector<uint8_t> outlineMask(width * height, 0);

    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            const size_t pixelIndex = y * width + x;
            if (m_DepthBuffer->data[pixelIndex] >= backgroundDepth) {
                continue;
            }

            for (int dy = -radius; dy <= radius; dy++) {
                const int ny = static_cast<int>(y) + dy;
                if (ny < 0 || ny >= static_cast<int>(height)) {
                    continue;
                }
                for (int dx = -radius; dx <= radius; dx++) {
                    if (dx == 0 && dy == 0) {
                        continue;
                    }
                    if (dx * dx + dy * dy > radius * radius) {
                        continue;
                    }

                    const int nx = static_cast<int>(x) + dx;
                    if (nx < 0 || nx >= static_cast<int>(width)) {
                        continue;
                    }

                    const size_t neighborIndex = static_cast<size_t>(ny) * width + static_cast<size_t>(nx);
                    if (m_DepthBuffer->data[neighborIndex] >= backgroundDepth) {
                        outlineMask[neighborIndex] = 1;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < outlineMask.size(); i++) {
        if (outlineMask[i] != 0) {
            m_FrameBuffer->data[i] = outlineColor;
        }
    }
}

void SWRenderer::DrawSkybox() {
    if (!m_HasSkybox || !p_Camera || !m_FrameBuffer) {
        return;
    }

    if (!m_SkyboxCacheValid ||
        !SkyboxCacheMatches(
            *p_Camera,
            m_FrameBuffer->width,
            m_FrameBuffer->height,
            m_SkyboxCacheCameraType,
            m_SkyboxCacheDirection,
            m_SkyboxCacheFov,
            m_SkyboxCacheOrthoSize,
            m_SkyboxCacheWidth,
            m_SkyboxCacheHeight)) {
        BuildSkyboxCache(
            m_SkyboxCachePixels,
            m_FrameBuffer->width,
            m_FrameBuffer->height,
            *p_Camera,
            m_SkyboxFaces,
            m_SkyboxFaceSize);
        m_SkyboxCacheCameraType = p_Camera->m_Type;
        m_SkyboxCacheDirection = glm::normalize(p_Camera->m_Direction);
        m_SkyboxCacheFov = p_Camera->m_Fov;
        m_SkyboxCacheOrthoSize = p_Camera->m_OrthoSize;
        m_SkyboxCacheWidth = m_FrameBuffer->width;
        m_SkyboxCacheHeight = m_FrameBuffer->height;
        m_SkyboxCacheValid = true;
    }

    if (m_SkyboxCachePixels.size() != m_FrameBuffer->GetCount()) {
        return;
    }
    std::copy_n(m_SkyboxCachePixels.data(), m_SkyboxCachePixels.size(), m_FrameBuffer->data);
}

void SWRenderer::DrawGridGizmo() {
    if (!p_Camera || !m_FrameBuffer || !m_DepthBuffer) {
        return;
    }

    const glm::mat4 viewProjection = p_Camera->m_ProjMat * p_Camera->m_ViewMat;
    const std::vector<GridGizmoVertex> gridVertices = BuildGridGizmoVertices(p_Camera->m_Position);
    for (size_t i = 0; i + 1 < gridVertices.size(); i += 2) {
        glm::vec4 clipStart = viewProjection * glm::vec4(gridVertices[i].position, 1.0f);
        glm::vec4 clipEnd = viewProjection * glm::vec4(gridVertices[i + 1].position, 1.0f);
        if (!ClipLineSegmentClipSpace(clipStart, clipEnd)) {
            continue;
        }
        if (std::abs(clipStart.w) <= 1e-6f || std::abs(clipEnd.w) <= 1e-6f) {
            continue;
        }

        const glm::vec3 ndcStart = glm::vec3(clipStart) / clipStart.w;
        const glm::vec3 ndcEnd = glm::vec3(clipEnd) / clipEnd.w;
        const glm::vec2 viewportStart = Rasterizer::NDCToViewport(glm::vec2(ndcStart), m_FrameBuffer->width, m_FrameBuffer->height);
        const glm::vec2 viewportEnd = Rasterizer::NDCToViewport(glm::vec2(ndcEnd), m_FrameBuffer->width, m_FrameBuffer->height);
        const glm::vec3 lineStart(viewportStart.x, viewportStart.y, ndcStart.z * 0.5f + 0.5f);
        const glm::vec3 lineEnd(viewportEnd.x, viewportEnd.y, ndcEnd.z * 0.5f + 0.5f);
        DrawDepthTestedLine(
            *m_FrameBuffer,
            *m_DepthBuffer,
            lineStart,
            lineEnd,
            m_FrameConfigSnapshot,
            GridColorToPixel(gridVertices[i].color));
    }
}

const Buffer<Pixel>& SWRenderer::GetFrameBuffer() const {
    assert(m_FrameBuffer != nullptr && "No render target set. Did you call SWRenderer::Init()?");
    return *m_FrameBuffer;
}
} // namespace RetroRenderer
