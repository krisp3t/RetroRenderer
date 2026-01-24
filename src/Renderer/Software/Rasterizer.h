#pragma once

#include "../../Base/Color.h"
#include "../../Base/Config.h"
#include "../../Scene/Vertex.h"
#include "../Buffer.h"
#include <array>

namespace RetroRenderer {

class Rasterizer {
public:
    Rasterizer() = default;
    ~Rasterizer() = default;
    // TODO: add configurable line/triangle colors
    static glm::vec2 NDCToViewport(const glm::vec2& v, size_t width, size_t height);
    static void DrawTriangle(Buffer<Pixel>& framebuffer,
                             Buffer<float>& depthBuffer,
                             std::array<Vertex, 3>& vertices,
                             Config& cfg,
                             Pixel fillColor);
    static void DrawQuad(Buffer<Pixel>& framebuffer, std::array<Vertex, 3>& vertices);
    static void DrawLine(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, Pixel color);
    static void DrawHLine(Buffer<Pixel>& framebuffer, int x0, int x1, int y, Pixel color);
    static void DrawPixel(Buffer<Pixel>& framebuffer, float x, float y, Pixel color);

private:
    static void DrawBarycentricTriangle(Buffer<Pixel>& framebuffer,
                                        Buffer<float>& depthBuffer,
                                        std::array<glm::vec3, 3>& viewportVertices,
                                        Config& cfg,
                                        Pixel fillColor);
    // Line drawing algos
    static void DrawLineDDA(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, Pixel color);
    static void DrawLineBresenham(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, Pixel color);
    // Point trig
    static void DrawPointTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec3, 3>& viewportVertices);
    // Wireframe trig
    static void DrawWireframeTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec3, 3>& viewportVertices);
    // Flat trig
    static void DrawFlatTriangle(Buffer<Pixel>& framebuffer,
                                 Buffer<float>& depthBuffer,
                                 std::array<glm::vec3, 3>& viewportVertices,
                                 Config& cfg,
                                 Pixel fillColor);
    static void FillFlatBottomTri(Buffer<Pixel>& framebuffer,
                                  Buffer<float>& depthBuffer,
                                  glm::vec3& v0,
                                  glm::vec3& v1,
                                  glm::vec3& v2,
                                  Config& cfg,
                                  Pixel fillColor);
    static void FillFlatTopTri(Buffer<Pixel>& framebuffer,
                               Buffer<float>& depthBuffer,
                               glm::vec3& v0,
                               glm::vec3& v1,
                               glm::vec3& v2,
                               Config& cfg,
                               Pixel fillColor);
    // Trig cull
    static bool PixelCullTriangle(const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2,
                                  const glm::vec2& testPoint);
    static bool IsTriangleDegenerate(std::array<glm::vec3, 3>& vertices);
    static bool IsBackface(std::array<glm::vec3, 3>& vertices);
};

} // namespace RetroRenderer
