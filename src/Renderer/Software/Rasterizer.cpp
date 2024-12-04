#include "Rasterizer.h"

namespace RetroRenderer
{
    glm::ivec2 Rasterizer::NDCToViewport(const glm::vec2 &v, int width, int height)
    {
        return {
                static_cast<int>((v.x + 1.0f) * 0.5f * width + 0.5f),
                static_cast<int>((1.0f - v.y) * 0.5f * height + 0.5f)
        };
    }

    void Rasterizer::DrawTriangle(Buffer<Uint32> &framebuffer, Vertex &v0, Vertex &v1, Vertex &v2)
    {
        // Sort vertices by y-coordinate
        if (v0.position.y > v1.position.y) std::swap(v0, v1);
        if (v0.position.y > v2.position.y) std::swap(v0, v2);
        if (v1.position.y > v2.position.y) std::swap(v1, v2);

        DrawWireframeTriangle(
            framebuffer,
            NDCToViewport(v0.position, framebuffer.width, framebuffer.height),
            NDCToViewport(v1.position, framebuffer.width, framebuffer.height),
            NDCToViewport(v2.position, framebuffer.width, framebuffer.height)
        );
    }

    void Rasterizer::DrawWireframeTriangle(Buffer<Uint32> &framebuffer, glm::vec2 v0, glm::vec2 v1, glm::vec2 v2)
    {
        // TODO: Add color selector
        DrawLine(framebuffer, v0, v1, 0xFF0000FF);
        DrawLine(framebuffer, v1, v2, 0x00FF00FF);
        DrawLine(framebuffer, v2, v0, 0x0000FFFF);
    }

    void Rasterizer::DrawLine(Buffer<Uint32> &framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color)
    {
        // DrawLineDDA(framebuffer, p0, p1, color);
        DrawLineBresenham(framebuffer, p0, p1, color);
    }

    void Rasterizer::DrawLineDDA(Buffer<Uint32> &framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color)
    {
        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        float steps = std::max(std::abs(dx), std::abs(dy));
        float xInc = dx / steps;
        float yInc = dy / steps;
        float x = p0.x;
        float y = p0.y;
        for (int i = 0; i <= steps; i++)
        {
            DrawPixel(framebuffer, x, y, color);
            x += xInc;
            y += yInc;
        }
    }

    void Rasterizer::DrawLineBresenham(Buffer<Uint32> &fb, glm::vec2 p0, glm::vec2 p1, Uint32 color)
    {
        bool steep = false;
        if (std::abs(p0.x - p1.x) < std::abs(p0.y - p1.y)) {
            std::swap(p0.x, p0.y);
            std::swap(p1.x, p1.y);
            steep = true;
        }
        if (p0.x > p1.x) {
            std::swap(p0.x, p1.x);
            std::swap(p0.y, p1.y);
        }
        int dx = p1.x - p0.x;
        int dy = p1.y - p0.y;
        int derror2 = std::abs(dy) * 2;
        int error2 = 0;
        int y = p0.y;
        for (int x = p0.x; x <= p1.x; x++) {
            if (steep) {
                DrawPixel(fb, y, x, color);
            }
            else {
                DrawPixel(fb, x, y, color);
            }
            error2 += derror2;
            if (error2 > dx) {
                y += (p1.y > p0.y ? 1 : -1);
                error2 -= dx * 2;
            }
        }
    }

    void Rasterizer::FillFlatBottomTri(Buffer<Uint32> &framebuffer, Vertex &v0, Vertex &v1, Vertex &mid)
    {

    }

    void Rasterizer::FillFlatTopTri(Buffer<Uint32> &framebuffer, Vertex &v1, Vertex &mid, Vertex &v2)
    {

    }

    void Rasterizer::DrawPixel(Buffer<Uint32> &framebuffer, int x, int y, Uint32 color)
    {
        assert(!(x < 0 || x >= framebuffer.width || y < 0 || y >= framebuffer.height) && "Pixel out of bounds");
		framebuffer.Set(x, y, color);
    }
}