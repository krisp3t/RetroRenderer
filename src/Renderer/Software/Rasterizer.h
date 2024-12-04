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
    static void DrawWireframeTriangle(Buffer <Uint32> &framebuffer, glm::vec2 v0, glm::vec2 v1, glm::vec2 v2);
    static void DrawLine(Buffer<Uint32> &framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color);
    static void DrawLineDDA(Buffer<Uint32> &framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color);
    static void DrawLineBresenham(Buffer<Uint32> &framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color);
    static void FillFlatBottomTri(Buffer<Uint32> &framebuffer, Vertex &v0, Vertex &v1, Vertex &mid);
    static void FillFlatTopTri(Buffer<Uint32> &framebuffer, Vertex &v1, Vertex &mid, Vertex &v2);
    static void DrawPixel(Buffer<Uint32> &framebuffer, int x, int y, Uint32 color);
    static glm::ivec2 NDCToViewport(const glm::vec2 &v, int width, int height);

private:

};

}

