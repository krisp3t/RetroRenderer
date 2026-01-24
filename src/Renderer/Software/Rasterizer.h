#pragma once

#include "../../Base/Config.h"
#include "../../Scene/Vertex.h"
#include "../Buffer.h"
#include <SDL.h>
#include <array>

namespace RetroRenderer {

class Rasterizer {
  public:
    Rasterizer() = default;
    ~Rasterizer() = default;
    // TODO: replace uint32 with color struct
    static glm::vec2 NDCToViewport(const glm::vec2 &v, int width, int height);
    static void DrawTriangle(Buffer<Uint32> &framebuffer, std::array<Vertex, 3> &vertices,
                             const Config::SoftwareRasterizerSettings &cfg);
    static void DrawQuad(Buffer<Uint32> &framebuffer, std::array<Vertex, 3> &vertices);
    static void DrawLine(Buffer<Uint32> &framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color);
    static void DrawHLine(Buffer<Uint32> &framebuffer, int x0, int x1, int y, Uint32 color);
    static void DrawPixel(Buffer<Uint32> &framebuffer, float x, float y, Uint32 color);

  private:
    // Line drawing algos
    static void DrawLineDDA(Buffer<Uint32> &framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color);
    static void DrawLineBresenham(Buffer<Uint32> &framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color);
    // Point trig
    static void DrawPointTriangle(Buffer<Uint32> &framebuffer, std::array<glm::vec2, 3> &viewportVertices);
    // Wireframe trig
    static void DrawWireframeTriangle(Buffer<Uint32> &framebuffer, std::array<glm::vec2, 3> &viewportVertices);
    // Flat trig
    static void DrawFlatTriangle(Buffer<Uint32> &framebuffer, std::array<glm::vec2, 3> &viewportVertices);
    static void FillFlatBottomTri(Buffer<Uint32> &framebuffer, glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2);
    static void FillFlatTopTri(Buffer<Uint32> &framebuffer, glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2);
    // Trig cull
    static bool PixelCullTriangle(const glm::vec2 &v0, const glm::vec2 &v1, const glm::vec2 &v2,
                                  const glm::vec2 &testPoint);
    static bool IsTriangleDegenerate(std::array<glm::vec2, 3> &vertices);
};

} // namespace RetroRenderer
