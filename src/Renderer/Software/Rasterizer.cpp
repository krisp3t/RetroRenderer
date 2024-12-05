#include <algorithm>
#include "../../Engine.h"
#include "Rasterizer.h"

namespace RetroRenderer
{
    glm::vec2 Rasterizer::NDCToViewport(const glm::vec2 &v, int width, int height)
    {
        return {
                static_cast<int>((v.x + 1.0f) * 0.5f * width + 0.5f),
                static_cast<int>((1.0f - v.y) * 0.5f * height + 0.5f)
        };
    }

	void Rasterizer::DrawLine(Buffer<Uint32>& framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color)
	{
		// DrawLineDDA(framebuffer, p0, p1, color);
		DrawLineBresenham(framebuffer, p0, p1, color);
	}

	void Rasterizer::DrawLineDDA(Buffer<Uint32>& framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color)
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

	void Rasterizer::DrawLineBresenham(Buffer<Uint32>& fb, glm::vec2 p0, glm::vec2 p1, Uint32 color)
	{
        // TODO: rasterization rules
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

    void Rasterizer::DrawTriangle(
        Buffer<Uint32> &framebuffer, 
        std::array<Vertex, 3>& vertices, 
        const Config::RasterizerSettings& cfg
        )
    {

		// Convert vertices to viewport space
        std::array<glm::vec2, 3> viewportVertices;
		for (int i = 0; i < 3; i++)
		{
			viewportVertices[i] = NDCToViewport(vertices[i].position, framebuffer.width, framebuffer.height);
		}

        // TODO: add cfg.lineColor
        switch (cfg.polygonMode)
        {
        case Config::RasterizationPolygonMode::POINT:
			DrawPointTriangle(framebuffer, viewportVertices);
			break;
		case Config::RasterizationPolygonMode::LINE:
			DrawWireframeTriangle(framebuffer, viewportVertices);
			break;
		case Config::RasterizationPolygonMode::FILL:
			DrawFlatTriangle(framebuffer, viewportVertices);
			break;
        }
    }

    void Rasterizer::DrawPointTriangle(Buffer<Uint32>& framebuffer, std::array<glm::vec2, 3>& viewportVertices)
    {

    }

	void Rasterizer::DrawWireframeTriangle(Buffer<Uint32>& framebuffer, std::array<glm::vec2, 3>& verts)
	{
		// TODO: Add color selector
		DrawLine(framebuffer, verts[0], verts[1], 0xFF0000FF);
		DrawLine(framebuffer, verts[1], verts[2], 0x00FF00FF);
		DrawLine(framebuffer, verts[2], verts[0], 0x0000FFFF);
	}

    void Rasterizer::DrawFlatTriangle(Buffer <Uint32>& framebuffer, std::array<glm::vec2, 3>& viewportVertices)
    {
		auto& v0 = viewportVertices[0];
		auto& v1 = viewportVertices[1];
		auto& v2 = viewportVertices[2];

		// Sort vertices by y-coordinate
		if (v0.y > v1.y) std::swap(v0, v1);
		if (v0.y > v2.y) std::swap(v0, v2);
		if (v1.y > v2.y) std::swap(v1, v2);
        /*

        // Split logic into flat-bottom and flat-top
        if (v1.y == v2.y)
        {
            FillFlatBottomTri(framebuffer, v0, v1, v2); // Flat-bottom
        }
        else if (v0.y == v1.y)
        {
            FillFlatTopTri(framebuffer, v0, v1, v2); // Flat-top
        }
        else
        {
            // Find the split point
            glm::ivec2 mid = {
                    v0.x + static_cast<int>((v1.y - v0.y) * static_cast<float>(v2.x - v0.x) / (v2.y - v0.y)),
                    v1.y
            };

            // Split into a flat-bottom and flat-top triangle
            FillFlatBottomTri(framebuffer, v0, v1, mid);
            FillFlatTopTri(framebuffer, v1, mid, v2);
        }
        */
    }

   
    /*
    void Rasterizer::FillFlatBottomTri(Buffer<Uint32> &framebuffer, glm::ivec2 &v0, glm::ivec2 &v1, glm::ivec2 &mid)
    {
        // Ensure floating-point division
        float invslope1 = static_cast<float>(v1.x - v0.x) / (v1.y - v0.y);
        float invslope2 = static_cast<float>(mid.x - v0.x) / (mid.y - v0.y);

        // Start and end points
        float xStart = v0.x;
        float xEnd = v0.x;
        int yStart = std::max(v0.y, 0);
        int yEnd = std::min(v1.y, static_cast<int>(framebuffer.height - 1));
        Uint32 color = 0xFFFFFFFF;

        // Scanline from top to flat line
        for (int y = yStart; y <= yEnd; y++)
        {
            DrawLine(framebuffer, { static_cast<int>(xStart), y }, { static_cast<int>(xEnd), y }, color);
            xStart += invslope1;
            xEnd += invslope2;
        }
    }

    void Rasterizer::FillFlatTopTri(Buffer<Uint32> &framebuffer, glm::ivec2 &v1, glm::ivec2 &mid, glm::ivec2 &v2)
    {
        // Ensure floating-point division
        float invslope1 = static_cast<float>(mid.x - v1.x) / (mid.y - v1.y);
        float invslope2 = static_cast<float>(v2.x - v1.x) / (v2.y - v1.y);

        // Start and end points
        float xStart = v1.x;
        float xEnd = v1.x;
        int yStart = std::max(v1.y, 0);
        int yEnd = std::min(v2.y, static_cast<int>(framebuffer.height - 1));
        Uint32 color = 0xFFFFFFFF;

        // Scanline from bottom to flat line
        for (int y = yStart; y <= yEnd; y++) // Increment y
        {
            DrawLine(framebuffer, { static_cast<int>(xStart), y }, { static_cast<int>(xEnd), y }, color);
            xStart += invslope1;
            xEnd += invslope2;
        }
    }
    */

    void Rasterizer::DrawPixel(Buffer<Uint32> &framebuffer, float x, float y, Uint32 color)
    {
        assert(!(x < 0 || x >= framebuffer.width || y < 0 || y >= framebuffer.height) && "Pixel out of bounds");
        // TODO: Rasterization rules
		framebuffer.Set(static_cast<int>(x), static_cast<int>(y), color);
    }
}