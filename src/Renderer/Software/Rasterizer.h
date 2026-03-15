#pragma once

#include "../../Base/Color.h"
#include "../../Base/Config.h"
#include "../../Scene/Light.h"
#include "../Buffer.h"
#include "SoftwareLighting.h"
#include <array>
#include <vector>

namespace RetroRenderer {
class Texture;

struct RasterVertex {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 cullPosition = glm::vec3(0.0f);
    glm::vec3 worldPosition = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 texCoords = glm::vec2(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float clipW = 1.0f;
};

class Rasterizer {
  public:
    Rasterizer() = default;
    ~Rasterizer() = default;
    // TODO: add configurable line/triangle colors
    static glm::vec2 NDCToViewport(const glm::vec2& v, size_t width, size_t height);
    static void DrawTriangle(Buffer<Pixel>& framebuffer,
                             Buffer<float>& depthBuffer,
                             std::array<RasterVertex, 3>& vertices,
                             const Config& cfg,
                             const std::vector<LightSnapshot>& lights,
                             const SoftwareMaterialState& materialState,
                             const glm::vec3& viewPosition,
                             const Texture* texture = nullptr);
    static void DrawQuad(Buffer<Pixel>& framebuffer, std::array<RasterVertex, 3>& vertices);
    static void DrawLine(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, const Config& cfg, Pixel color);
    static void DrawHLine(Buffer<Pixel>& framebuffer, int x0, int x1, int y, const Config& cfg, Pixel color);
    static void DrawPixel(Buffer<Pixel>& framebuffer, float x, float y, bool rasterClip, Pixel color);

  private:
    static void DrawBarycentricTriangle(Buffer<Pixel>& framebuffer,
                                        Buffer<float>& depthBuffer,
                                        const std::array<RasterVertex, 3>& vertices,
                                        std::array<glm::vec3, 3>& viewportVertices,
                                        const Config& cfg,
                                        const std::vector<LightSnapshot>& lights,
                                        const SoftwareMaterialState& materialState,
                                        const glm::vec3& viewPosition,
                                        const Texture* texture);
    // Line drawing algos
    static void DrawLineDDA(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, const Config& cfg, Pixel color);
    static void DrawLineBresenham(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, const Config& cfg, Pixel color);
    // Point trig
    static void DrawPointTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec3, 3>& viewportVertices, const Config& cfg);
    // Wireframe trig
    static void DrawWireframeTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec3, 3>& viewportVertices, const Config& cfg);
    // Flat trig
    static void DrawFlatTriangle(Buffer<Pixel>& framebuffer,
                                 Buffer<float>& depthBuffer,
                                 std::array<glm::vec3, 3>& viewportVertices,
                                 const Config& cfg,
                                 Pixel fillColor);
    static void FillFlatBottomTri(Buffer<Pixel>& framebuffer,
                                  Buffer<float>& depthBuffer,
                                  glm::vec3& v0,
                                  glm::vec3& v1,
                                  glm::vec3& v2,
                                  const Config& cfg,
                                  Pixel fillColor,
                                  const std::array<Pixel, 16>& fillPattern);
    static void FillFlatTopTri(Buffer<Pixel>& framebuffer,
                               Buffer<float>& depthBuffer,
                               glm::vec3& v0,
                               glm::vec3& v1,
                               glm::vec3& v2,
                               const Config& cfg,
                               Pixel fillColor,
                               const std::array<Pixel, 16>& fillPattern);
    // Trig cull
    static bool PixelCullTriangle(const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2,
                                  const glm::vec2& testPoint);
    static bool IsTriangleDegenerate(std::array<glm::vec3, 3>& vertices);
    static bool IsBackface(std::array<glm::vec3, 3>& vertices);
};

} // namespace RetroRenderer
