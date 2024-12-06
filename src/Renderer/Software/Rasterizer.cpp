#include <algorithm>
#include "../../Engine.h"
#include "Rasterizer.h"

// TODO: possible parallel rasterizing?

namespace RetroRenderer
{
    glm::vec2 Rasterizer::NDCToViewport(const glm::vec2 &v, int width, int height)
    {
        return {
                static_cast<int>((v.x + 1.0f) * 0.5f * width + 0.5f),
                static_cast<int>((1.0f - v.y) * 0.5f * height + 0.5f)
        };
    }

	void Rasterizer::DrawHLine(Buffer<Uint32>& framebuffer, int x0, int x1, int y, Uint32 color)
	{
		assert(framebuffer.height > y && "Y out of bounds");
		assert(x0 >= 0 && x0 < framebuffer.width && "X0 out of bounds");
		assert(x1 >= 0 && x1 < framebuffer.width && "X1 out of bounds");

		if (x0 > x1) std::swap(x0, x1);
		int startIndex = y * framebuffer.width + x0;
		std::fill_n(&framebuffer.data[startIndex], x1 - x0 + 1, color);
	}

	void Rasterizer::DrawLine(Buffer<Uint32>& framebuffer, glm::vec2 p0, glm::vec2 p1, Uint32 color)
	{
		switch (Engine::Get().GetConfig()->rasterizer.lineMode)
		{
		case Config::RasterizationLineMode::DDA:
			DrawLineDDA(framebuffer, p0, p1, color);
			break;
		case Config::RasterizationLineMode::BRESENHAM:
			DrawLineBresenham(framebuffer, p0, p1, color);
			break;
		}
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
            /*
            cfg.basicLineColors ?
                DrawWireframeTriangle(framebuffer, viewportVertices) :
                DrawWireframeTriangle(framebuffer, viewportVertices, cfg.lineColor);
                */
			DrawWireframeTriangle(framebuffer, viewportVertices);
			break;
		case Config::RasterizationPolygonMode::FILL:
			DrawFlatTriangle(framebuffer, viewportVertices);
			break;
        }
    }

    void Rasterizer::DrawPointTriangle(Buffer<Uint32>& framebuffer, std::array<glm::vec2, 3>& viewportVertices)
    {
		DrawPixel(framebuffer, viewportVertices[0].x, viewportVertices[0].y, 0xFFFFFFFF);
		DrawPixel(framebuffer, viewportVertices[1].x, viewportVertices[1].y, 0xFFFFFFFF);
		DrawPixel(framebuffer, viewportVertices[2].x, viewportVertices[2].y, 0xFFFFFFFF);
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
  
        // Flat-bottom triangle
        if (v1.y == v2.y)
        {
			if (v2.x < v1.x) std::swap(v2, v1); // Ensure v1 is leftmost
            FillFlatBottomTri(framebuffer, v0, v1, v2);
            return;
        }
        // Flat-top triangle
        if (v0.y == v1.y)
        {
			if (v1.x < v0.x) std::swap(v1, v0); // Ensure v2 is rightmost
            FillFlatTopTri(framebuffer, v0, v1, v2);
            return;
        }
        // Neither, need to split triangle
		// Find midpoint with linear interpolation
		const float alpha = (v1.y - v0.y) / (v2.y - v0.y);
        glm::vec2 mid = {
			v0.x + alpha * (v2.x - v0.x),
			v1.y
		}; 

        // Split into a flat-bottom and flat-top triangle
		if (v1.x < mid.x) // Major-right triangle
		{
			FillFlatBottomTri(framebuffer, v0, v1, mid);
			FillFlatTopTri(framebuffer, v1, mid, v2);
		}
		else // Major-left triangle
		{
			FillFlatBottomTri(framebuffer, v0, mid, v1);
			FillFlatTopTri(framebuffer, mid, v1, v2);
		}
    }

	/**
	 * @brief Rasterizes a flat-bottom triangle (flat side: v0-v1)
	 */
    void Rasterizer::FillFlatBottomTri(Buffer<Uint32> &framebuffer, glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2)
    {
		// Calculate invslopes in screen space
		// Run over rise, because edges can be completely vertical (infinite slope)
		float invslope1 = (v1.x - v0.x) / (v1.y - v0.y);
		float invslope2 = (v2.x - v0.x) / (v2.y - v0.y);

		// Start and end scanlines
		const int yStart = static_cast<int>(ceil(v0.y - 0.5f));
		const int yEnd = static_cast<int>(ceil(v2.y - 0.5f));
		const Uint32 color = 0xFFFFFFFF;

		float currentX1 = v0.x;
		float currentX2 = v0.x;

		for (int y = yStart; y < yEnd; y++)
		{
			DrawHLine(
				framebuffer,
				static_cast<int>(currentX1),
				static_cast<int>(currentX2),
				y,
				color
			);
			currentX1 += invslope1;
			currentX2 += invslope2;
		}
    }

	/**
	 * @brief Rasterizes a flat-top triangle (flat side: v1-v2)
	 */
    void Rasterizer::FillFlatTopTri(Buffer<Uint32> &framebuffer, glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2)
    {
        // Calculate invslopes in screen space
		// Run over rise, because edges can be completely vertical (infinite slope)
		float invslope1 = (v2.x - v0.x) / (v2.y - v0.y);
		float invslope2 = (v2.x - v1.x) / (v2.y - v1.y);

		// Start and end scanlines
		const int yStart = static_cast<int>(ceil(v0.y - 0.5f));
		const int yEnd = static_cast<int>(ceil(v2.y - 0.5f));
		const Uint32 color = 0xFFFFFFFF;

		float currentX1 = v2.x;
		float currentX2 = v2.x;

		for (int y = yStart; y < yEnd; y--)
		{
			DrawHLine(
				framebuffer, 
				static_cast<int>(currentX1), 
				static_cast<int>(currentX2), 
				y, 
				color
			);
			currentX1 -= invslope1;
			currentX2 -= invslope2;
		}
    }

    void Rasterizer::DrawPixel(Buffer<Uint32> &framebuffer, float x, float y, Uint32 color)
    {
        assert(!(x < 0 || x >= framebuffer.width || y < 0 || y >= framebuffer.height) && "Pixel out of bounds");
        // TODO: Rasterization rules
		framebuffer.Set(static_cast<int>(x), static_cast<int>(y), color);
    }
}