#include "Rasterizer.h"
#include "../../Engine.h"
#include <KrisLogger/Logger.h>
#include <algorithm>

// TODO: possible parallel rasterizing?

namespace RetroRenderer {
glm::vec2 Rasterizer::NDCToViewport(const glm::vec2& v, size_t width, size_t height) {
    return {static_cast<int>((v.x + 1.0f) * 0.5f * width + 0.5f),
            static_cast<int>((1.0f - v.y) * 0.5f * height + 0.5f)};
}

void Rasterizer::DrawHLine(Buffer<Pixel>& framebuffer, int x0, int x1, int y, Pixel color) {
    assert(y >= 0 && x0 >= 0 && x1 >= 0);
    assert(framebuffer.height > static_cast<size_t>(y) && "Y out of bounds");

    if (x0 > x1)
        std::swap(x0, x1);
    int startIndex = y * framebuffer.width + x0;
    int length = x1 - x0 + 1;

    assert(static_cast<size_t>(x0) < framebuffer.width && "X0 out of bounds");
    assert(static_cast<size_t>(x1) < framebuffer.width && "X1 out of bounds");
    assert(startIndex >= 0 && static_cast<size_t>(startIndex) < framebuffer.GetSize() && "Start index out of bounds");
    assert(static_cast<size_t>(startIndex + length) <= framebuffer.GetSize() && "End index out of bounds");

    std::fill_n(&framebuffer.data[startIndex], length, color);
}

/**
 * @brief Draws a line between two points using the selected line drawing algorithm
 * @param framebuffer The framebuffer to draw on
 * @param p0 The start point in screen space
 * @param p1 The end point in screen space
 * @param color The color of the line
 */
void Rasterizer::DrawLine(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, Pixel color) {
    switch (Engine::Get().GetConfig()->software.rasterizer.lineMode) {
    case Config::RasterizationLineMode::DDA:
        DrawLineDDA(framebuffer, p0, p1, color);
        break;
    case Config::RasterizationLineMode::BRESENHAM:
        DrawLineBresenham(framebuffer, p0, p1, color);
        break;
    }
}

void Rasterizer::DrawLineDDA(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, Pixel color) {
    float dx = p1.x - p0.x;
    float dy = p1.y - p0.y;
    float steps = std::max(std::abs(dx), std::abs(dy));
    float xInc = dx / steps;
    float yInc = dy / steps;
    float x = p0.x;
    float y = p0.y;
    for (int i = 0; i <= steps; i++) {
        if (x < 0 || x >= framebuffer.width || y < 0 || y >= framebuffer.height)
            break;
        DrawPixel(framebuffer, x, y, color);
        x += xInc;
        y += yInc;
    }
}

void Rasterizer::DrawLineBresenham(Buffer<Pixel>& fb, glm::vec2 p0, glm::vec2 p1, Pixel color) {
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
        steep ? DrawPixel(fb, y, x, color) : DrawPixel(fb, x, y, color);
        error2 += derror2;
        if (error2 > dx) {
            y += (p1.y > p0.y ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

void Rasterizer::DrawTriangle(Buffer<Pixel>& framebuffer, std::array<Vertex, 3>& vertices,
                              const Config::SoftwareRasterizerSettings& cfg) {

    // Convert vertices to viewport space
    std::array<glm::vec2, 3> viewportVertices{};
    for (int i = 0; i < 3; i++) {
        viewportVertices[i] = NDCToViewport(vertices[i].position, framebuffer.width, framebuffer.height);
    }

    // TODO: add cfg.lineColor
    switch (cfg.polygonMode) {
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
    default:
        LOGW("Invalid polygon mode");
        break;
    }
}

void Rasterizer::DrawPointTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec2, 3>& viewportVertices) {
    const Pixel white{255, 255, 255, 255};
    DrawPixel(framebuffer, viewportVertices[0].x, viewportVertices[0].y, white);
    DrawPixel(framebuffer, viewportVertices[1].x, viewportVertices[1].y, white);
    DrawPixel(framebuffer, viewportVertices[2].x, viewportVertices[2].y, white);
}

void Rasterizer::DrawWireframeTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec2, 3>& verts) {
    // TODO: Add color selector
    DrawLine(framebuffer, verts[0], verts[1], Pixel{255, 0, 0, 255});
    DrawLine(framebuffer, verts[1], verts[2], Pixel{0, 255, 0, 255});
    DrawLine(framebuffer, verts[2], verts[0], Pixel{0, 0, 255, 255});
}

/**
 * @brief Check if point lies inside triangle (barycentric coords)
 */
bool Rasterizer::PixelCullTriangle(const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2,
                                   const glm::vec2& testPoint) {
    float areaTotal = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);

    // Cull if the triangle is degenerate (zero area)
    if (areaTotal == 0.0f)
        return true;

    // Calculate barycentric coordinates for the test point
    float alpha =
        ((v1.x - testPoint.x) * (v2.y - testPoint.y) - (v1.y - testPoint.y) * (v2.x - testPoint.x)) / areaTotal;
    float beta =
        ((v2.x - testPoint.x) * (v0.y - testPoint.y) - (v2.y - testPoint.y) * (v0.x - testPoint.x)) / areaTotal;
    float gamma =
        ((v0.x - testPoint.x) * (v1.y - testPoint.y) - (v0.y - testPoint.y) * (v1.x - testPoint.x)) / areaTotal;

    // Cull the triangle if any barycentric coordinate is negative
    return (alpha < 0.0f || beta < 0.0f || gamma < 0.0f);
}

/**
 * @brief Check if the triangle is degenerate (zero area)
 */
bool Rasterizer::IsTriangleDegenerate(std::array<glm::vec2, 3>& vertices) {
    // Cross product is zero exactly when:
    // 1. Two vectors are parallel/anti-parallel (linearly dependent)
    // 2. One of the vectors is zero
    return glm::cross(glm::vec3(vertices[1] - vertices[0], 0.0f), glm::vec3(vertices[2] - vertices[0], 0.0f)).z == 0.0f;
}

void Rasterizer::DrawFlatTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec2, 3>& viewportVertices) {
    auto& v0 = viewportVertices[0];
    auto& v1 = viewportVertices[1];
    auto& v2 = viewportVertices[2];

    auto& p_Config = Engine::Get().GetConfig();

    // Sort vertices by y-coordinate
    if (v0.y > v1.y)
        std::swap(v0, v1);
    if (v0.y > v2.y)
        std::swap(v0, v2);
    if (v1.y > v2.y)
        std::swap(v1, v2);

    // Skip degenerate triangles (zero area)
    // Done in backface culling already
    if (!p_Config->cull.backfaceCulling) {
        if (IsTriangleDegenerate(viewportVertices))
            return;
    }

    // Flat-bottom triangle
    if (v1.y == v2.y) {
        if (v2.x < v1.x)
            std::swap(v2, v1); // Ensure v1 is leftmost
        FillFlatBottomTri(framebuffer, v0, v1, v2);
        return;
    }
    // Flat-top triangle
    if (v0.y == v1.y) {
        if (v1.x < v0.x)
            std::swap(v1, v0); // Ensure v2 is rightmost
        FillFlatTopTri(framebuffer, v0, v1, v2);
        return;
    }
    // Neither, need to split triangle
    // Find midpoint with linear interpolation
    const float alpha = (v1.y - v0.y) / (v2.y - v0.y);
    glm::vec2 mid = {v0.x + alpha * (v2.x - v0.x), v1.y};

    // Split into a flat-bottom and flat-top triangle
    if (v1.x < mid.x) // Major-right triangle
    {
        FillFlatBottomTri(framebuffer, v0, v1, mid);
        FillFlatTopTri(framebuffer, v1, mid, v2);
    } else // Major-left triangle
    {
        FillFlatBottomTri(framebuffer, v0, mid, v1);
        FillFlatTopTri(framebuffer, mid, v1, v2);
    }
}

/**
 * @brief Rasterizes a flat-bottom triangle (flat side: v0-v1)
 */
void Rasterizer::FillFlatBottomTri(Buffer<Pixel>& framebuffer, glm::vec2& v0, glm::vec2& v1, glm::vec2& v2) {
    // Calculate invslopes in screen space
    // Run over rise, because edges can be completely vertical (infinite slope)
    double invslope1 = (v1.x - v0.x) / (v1.y - v0.y);
    double invslope2 = (v2.x - v0.x) / (v2.y - v0.y);

    // Start and end scanlines
    const int yStart = std::max(0, static_cast<int>(ceil(v0.y - 0.5f)));
    const int yEnd = std::min(static_cast<int>(ceil(v2.y - 0.5f)), static_cast<int>(framebuffer.height - 1));
    const Pixel color{255, 255, 255, 255};

    float currentX1 = v0.x;
    float currentX2 = v0.x;

    for (int y = yStart; y < yEnd && currentX1 <= currentX2; y++) {
        // TODO: add raster clip toggle

        // Raster clipping
        const int xStart = std::max(0, static_cast<int>(currentX1));
        const int xEnd = std::min(static_cast<int>(currentX2), static_cast<int>(framebuffer.width - 1));

        for (int x = xStart; x <= xEnd; x++) {
            // TODO: depth test
            DrawPixel(framebuffer, x, y, color);
        }
        currentX1 += invslope1;
        currentX2 += invslope2;
    }
}

/**
 * @brief Rasterizes a flat-top triangle (flat side: v1-v2)
 */
void Rasterizer::FillFlatTopTri(Buffer<Pixel>& framebuffer, glm::vec2& v0, glm::vec2& v1, glm::vec2& v2) {
    // Calculate invslopes in screen space
    // Run over rise, because edges can be completely vertical (infinite slope)
    double invslope1 = (v2.x - v0.x) / (v2.y - v0.y);
    double invslope2 = (v2.x - v1.x) / (v2.y - v1.y);

    // Start and end scanlines
    const int yStart = std::min(static_cast<int>(ceil(v0.y - 0.5f)), static_cast<int>(framebuffer.height - 1));
    const int yEnd = std::max(0, static_cast<int>(ceil(v2.y - 0.5f)));
    const Pixel color{255, 255, 255, 255};

    float currentX1 = v2.x;
    float currentX2 = v2.x;

    for (int y = yStart; y > yEnd && currentX1 <= currentX2; y--) {
        // TODO: add raster clip toggle

        // Raster clipping
        const int xStart = std::max(0, static_cast<int>(currentX1));
        const int xEnd = std::min(static_cast<int>(currentX2), static_cast<int>(framebuffer.width - 1));

        for (int x = xStart; x <= xEnd; x++) {
            // TODO: depth test
            DrawPixel(framebuffer, x, y, color);
        }
        currentX1 -= invslope1;
        currentX2 -= invslope2;
    }
}

void Rasterizer::DrawPixel(Buffer<Pixel>& framebuffer, float x, float y, Pixel color) {
    if (x < 0 || x >= framebuffer.width || y < 0 || y >= framebuffer.height)
        return;
    // TODO: Rasterization rules
    framebuffer.Set(static_cast<int>(x), static_cast<int>(y), color);
}
} // namespace RetroRenderer
