#pragma once
#include <SDL.h>
#include "../Buffer.h"
#include "../Vertex.h"

namespace RetroRenderer
{

class Rasterizer
{
public:
    Rasterizer() = default;
    ~Rasterizer() = default;
    static void DrawTriangle(Buffer<Uint32> &framebuffer, Vertex &v0, Vertex &v1, Vertex &v2);
    static void DrawQuad(Buffer<Uint32> &framebuffer, Vertex &v0, Vertex &v1, Vertex &v2, Vertex &v3);
    static void DrawLine(Buffer<Uint32> &framebuffer, glm::ivec2 p0, glm::ivec2 p1, Uint32 color);
    static void DrawPixel(Buffer<Uint32> &framebuffer, int x, int y, Uint32 color);
    static glm::ivec2 NDCToViewport(const glm::vec2 &v, int width, int height);

private:
    static void DrawLineDDA(Buffer<Uint32> &framebuffer, glm::ivec2 p0, glm::ivec2 p1, Uint32 color);
    static void DrawLineBresenham(Buffer<Uint32> &framebuffer, glm::ivec2 p0, glm::ivec2 p1, Uint32 color);
    static void DrawWireframeTriangle(Buffer <Uint32> &framebuffer, glm::ivec2 v0, glm::ivec2 v1, glm::ivec2 v2);
    static void DrawFlatTriangle(Buffer <Uint32> &framebuffer, glm::ivec2 v0, glm::ivec2 v1, glm::ivec2 v2);
    static void FillFlatBottomTri(Buffer<Uint32> &framebuffer, glm::ivec2 &v0, glm::ivec2 &v1, glm::ivec2 &mid);
    static void FillFlatTopTri(Buffer<Uint32> &framebuffer, glm::ivec2 &v1, glm::ivec2 &mid, glm::ivec2 &v2);
};

}

