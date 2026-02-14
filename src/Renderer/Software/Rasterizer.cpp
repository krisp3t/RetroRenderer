#include "Rasterizer.h"
#include <KrisLogger/Logger.h>
#include <algorithm>
#include <cmath>

// TODO: possible parallel rasterizing?

namespace RetroRenderer {
glm::vec2 Rasterizer::NDCToViewport(const glm::vec2& v, size_t width, size_t height) {
    // Keep subpixel precision for raster rules.
    return {(v.x + 1.0f) * 0.5f * width + 0.5f, (1.0f - v.y) * 0.5f * height + 0.5f};
}

void Rasterizer::DrawHLine(Buffer<Pixel>& framebuffer, int x0, int x1, int y, const Config& cfg, Pixel color) {
    const bool rasterClip = cfg.cull.rasterClip;
    if (rasterClip) {
        if (y < 0 || x0 < 0 || x1 < 0 || static_cast<size_t>(y) >= framebuffer.height) {
            return;
        }
    }

    if (x0 > x1)
        std::swap(x0, x1);
    if (rasterClip) {
        x0 = std::max(0, x0);
        x1 = std::min(static_cast<int>(framebuffer.width - 1), x1);
    }
    int startIndex = y * framebuffer.width + x0;
    int length = x1 - x0 + 1;

    if (rasterClip && length <= 0) {
        return;
    }

    std::fill_n(&framebuffer.data[startIndex], length, color);
}

/**
 * @brief Draws a line between two points using the selected line drawing algorithm
 * @param framebuffer The framebuffer to draw on
 * @param p0 The start point in screen space
 * @param p1 The end point in screen space
 * @param color The color of the line
 */
void Rasterizer::DrawLine(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, const Config& cfg, Pixel color) {
    switch (cfg.software.rasterizer.lineMode) {
    case Config::RasterizationLineMode::DDA:
        DrawLineDDA(framebuffer, p0, p1, cfg, color);
        break;
    case Config::RasterizationLineMode::BRESENHAM:
        DrawLineBresenham(framebuffer, p0, p1, cfg, color);
        break;
    }
}

void Rasterizer::DrawLineDDA(Buffer<Pixel>& framebuffer, glm::vec2 p0, glm::vec2 p1, const Config& cfg, Pixel color) {
    float dx = p1.x - p0.x;
    float dy = p1.y - p0.y;
    float steps = std::max(std::abs(dx), std::abs(dy));
    float xInc = dx / steps;
    float yInc = dy / steps;
    float x = p0.x;
    float y = p0.y;
    const bool rasterClip = cfg.cull.rasterClip;
    for (int i = 0; i <= steps; i++) {
        if (rasterClip) {
            if (x < 0 || x >= framebuffer.width || y < 0 || y >= framebuffer.height) {
                break;
            }
        }
        DrawPixel(framebuffer, x, y, rasterClip, color);
        x += xInc;
        y += yInc;
    }
}

void Rasterizer::DrawLineBresenham(Buffer<Pixel>& fb, glm::vec2 p0, glm::vec2 p1, const Config& cfg, Pixel color) {
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
        steep ? DrawPixel(fb, y, x, cfg.cull.rasterClip, color) : DrawPixel(fb, x, y, cfg.cull.rasterClip, color);
        error2 += derror2;
        if (error2 > dx) {
            y += (p1.y > p0.y ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

void Rasterizer::DrawTriangle(Buffer<Pixel>& framebuffer,
                              Buffer<float>& depthBuffer,
                              std::array<Vertex, 3>& vertices,
                              const Config& cfg,
                              Pixel fillColor) {

    // Convert vertices to viewport space.
    std::array<glm::vec3, 3> viewportVertices{};
    for (int i = 0; i < 3; i++) {
        const glm::vec2 viewportPos = NDCToViewport(vertices[i].position, framebuffer.width, framebuffer.height);
        // Map NDC z [-1, 1] into depth [0, 1].
        const float depth = vertices[i].position.z * 0.5f + 0.5f;
        viewportVertices[i] = glm::vec3(viewportPos.x, viewportPos.y, depth);
    }

    // Backface cull in screen space (front face = clockwise, D3D-style).
    if (cfg.cull.backfaceCulling) {
        if (IsTriangleDegenerate(viewportVertices) || IsBackface(viewportVertices)) {
            return;
        }
    }

    // TODO: add cfg.lineColor
    switch (cfg.software.rasterizer.polygonMode) {
    case Config::RasterizationPolygonMode::POINT:
        DrawPointTriangle(framebuffer, viewportVertices, cfg);
        break;
    case Config::RasterizationPolygonMode::LINE:
        /*
        cfg.basicLineColors ?
            DrawWireframeTriangle(framebuffer, viewportVertices) :
            DrawWireframeTriangle(framebuffer, viewportVertices, cfg.lineColor);
            */
        DrawWireframeTriangle(framebuffer, viewportVertices, cfg);
        break;
    case Config::RasterizationPolygonMode::FILL:
        switch (cfg.software.rasterizer.fillMode) {
        case Config::RasterizationFillMode::BARYCENTRIC:
            DrawBarycentricTriangle(framebuffer, depthBuffer, viewportVertices, cfg, fillColor);
            break;
        default:
            DrawFlatTriangle(framebuffer, depthBuffer, viewportVertices, cfg, fillColor);
            break;
        }
        break;
    default:
        LOGW("Invalid polygon mode");
        break;
    }
}

static float EdgeFunction(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

static bool IsTopLeftEdge(const glm::vec2& a, const glm::vec2& b) {
    const float dy = b.y - a.y;
    const float dx = b.x - a.x;
    return (dy < 0.0f) || (dy == 0.0f && dx > 0.0f);
}

void Rasterizer::DrawBarycentricTriangle(Buffer<Pixel>& framebuffer,
                                         Buffer<float>& depthBuffer,
                                         std::array<glm::vec3, 3>& viewportVertices,
                                         const Config& cfg,
                                         Pixel fillColor) {
    glm::vec2 v0 = {viewportVertices[0].x, viewportVertices[0].y};
    glm::vec2 v1 = {viewportVertices[1].x, viewportVertices[1].y};
    glm::vec2 v2 = {viewportVertices[2].x, viewportVertices[2].y};

    float area = EdgeFunction(v0, v1, v2);
    if (area == 0.0f) {
        return;
    }
    if (area < 0.0f) {
        std::swap(v1, v2);
        std::swap(viewportVertices[1], viewportVertices[2]);
        area = -area;
    }

    const bool rasterClip = cfg.cull.rasterClip;
    int minX = static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x}) - 0.5f));
    int minY = static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y}) - 0.5f));
    int maxX = static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x}) - 0.5f));
    int maxY = static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y}) - 0.5f));
    if (rasterClip) {
        minX = std::max(0, minX);
        minY = std::max(0, minY);
        maxX = std::min(maxX, static_cast<int>(framebuffer.width - 1));
        maxY = std::min(maxY, static_cast<int>(framebuffer.height - 1));
    }

    const bool e0TopLeft = IsTopLeftEdge(v1, v2);
    const bool e1TopLeft = IsTopLeftEdge(v2, v0);
    const bool e2TopLeft = IsTopLeftEdge(v0, v1);
    const float invArea = 1.0f / area;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            const glm::vec2 p = {static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
            float w0 = EdgeFunction(v1, v2, p);
            float w1 = EdgeFunction(v2, v0, p);
            float w2 = EdgeFunction(v0, v1, p);

            const bool inside =
                (w0 > 0.0f || (w0 == 0.0f && e0TopLeft)) &&
                (w1 > 0.0f || (w1 == 0.0f && e1TopLeft)) &&
                (w2 > 0.0f || (w2 == 0.0f && e2TopLeft));
            if (!inside) {
                continue;
            }

            const float b0 = w0 * invArea;
            const float b1 = w1 * invArea;
            const float b2 = w2 * invArea;
            const float z = viewportVertices[0].z * b0 + viewportVertices[1].z * b1 + viewportVertices[2].z * b2;

            if (!cfg.cull.depthTest || z < depthBuffer.data[y * depthBuffer.width + x]) {
                if (cfg.cull.depthTest) {
                    depthBuffer.data[y * depthBuffer.width + x] = z;
                }
                DrawPixel(framebuffer, x, y, rasterClip, fillColor);
            }
        }
    }
}

void Rasterizer::DrawPointTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec3, 3>& viewportVertices, const Config& cfg) {
    const Pixel white{255, 255, 255, 255};
    const bool rasterClip = cfg.cull.rasterClip;
    DrawPixel(framebuffer, viewportVertices[0].x, viewportVertices[0].y, rasterClip, white);
    DrawPixel(framebuffer, viewportVertices[1].x, viewportVertices[1].y, rasterClip, white);
    DrawPixel(framebuffer, viewportVertices[2].x, viewportVertices[2].y, rasterClip, white);
}

void Rasterizer::DrawWireframeTriangle(Buffer<Pixel>& framebuffer, std::array<glm::vec3, 3>& verts, const Config& cfg) {
    // TODO: Add color selector
    DrawLine(framebuffer, glm::vec2(verts[0].x, verts[0].y), glm::vec2(verts[1].x, verts[1].y), cfg, Pixel{255, 0, 0, 255});
    DrawLine(framebuffer, glm::vec2(verts[1].x, verts[1].y), glm::vec2(verts[2].x, verts[2].y), cfg, Pixel{0, 255, 0, 255});
    DrawLine(framebuffer, glm::vec2(verts[2].x, verts[2].y), glm::vec2(verts[0].x, verts[0].y), cfg, Pixel{0, 0, 255, 255});
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
bool Rasterizer::IsTriangleDegenerate(std::array<glm::vec3, 3>& vertices) {
    // Cross product is zero exactly when:
    // 1. Two vectors are parallel/anti-parallel (linearly dependent)
    // 2. One of the vectors is zero
    return glm::cross(glm::vec3(vertices[1] - vertices[0]), glm::vec3(vertices[2] - vertices[0])).z == 0.0f;
}

bool Rasterizer::IsBackface(std::array<glm::vec3, 3>& vertices) {
    const glm::vec2& v0 = vertices[0];
    const glm::vec2& v1 = vertices[1];
    const glm::vec2& v2 = vertices[2];
    const float area = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
    return area <= 0.0f;
}

void Rasterizer::DrawFlatTriangle(Buffer<Pixel>& framebuffer,
                                  Buffer<float>& depthBuffer,
                                  std::array<glm::vec3, 3>& viewportVertices,
                                  const Config& cfg,
                                  Pixel fillColor) {
    auto& v0 = viewportVertices[0];
    auto& v1 = viewportVertices[1];
    auto& v2 = viewportVertices[2];

    // Sort vertices by y-coordinate for flat-triangle splits.
    if (v0.y > v1.y)
        std::swap(v0, v1);
    if (v0.y > v2.y)
        std::swap(v0, v2);
    if (v1.y > v2.y)
        std::swap(v1, v2);

    // Flat-bottom triangle
    if (v1.y == v2.y) {
        if (v2.x < v1.x)
            std::swap(v2, v1); // Ensure v1 is leftmost
        FillFlatBottomTri(framebuffer, depthBuffer, v0, v1, v2, cfg, fillColor);
        return;
    }
    // Flat-top triangle
    if (v0.y == v1.y) {
        if (v1.x < v0.x)
            std::swap(v1, v0); // Ensure v2 is rightmost
        FillFlatTopTri(framebuffer, depthBuffer, v0, v1, v2, cfg, fillColor);
        return;
    }
    // Neither, need to split triangle
    // Find midpoint with linear interpolation
    const float alpha = (v1.y - v0.y) / (v2.y - v0.y);
    // Split point along v0->v2 at v1.y (interpolate z too).
    glm::vec3 mid = {v0.x + alpha * (v2.x - v0.x), v1.y, v0.z + alpha * (v2.z - v0.z)};

    // Split into a flat-bottom and flat-top triangle
    if (v1.x < mid.x) // Major-right triangle
    {
        FillFlatBottomTri(framebuffer, depthBuffer, v0, v1, mid, cfg, fillColor);
        FillFlatTopTri(framebuffer, depthBuffer, v1, mid, v2, cfg, fillColor);
    } else // Major-left triangle
    {
        FillFlatBottomTri(framebuffer, depthBuffer, v0, mid, v1, cfg, fillColor);
        FillFlatTopTri(framebuffer, depthBuffer, mid, v1, v2, cfg, fillColor);
    }
}

/**
 * @brief Rasterizes a flat-bottom triangle (flat side: v0-v1)
 */
void Rasterizer::FillFlatBottomTri(Buffer<Pixel>& framebuffer,
                                   Buffer<float>& depthBuffer,
                                   glm::vec3& v0,
                                   glm::vec3& v1,
                                   glm::vec3& v2,
                                   const Config& cfg,
                                   Pixel fillColor) {
    // Calculate invslopes in screen space
    // Run over rise, because edges can be completely vertical (infinite slope)
    double invslope1 = (v1.x - v0.x) / (v1.y - v0.y);
    double invslope2 = (v2.x - v0.x) / (v2.y - v0.y);
    double invslopez1 = (v1.z - v0.z) / (v1.y - v0.y);
    double invslopez2 = (v2.z - v0.z) / (v2.y - v0.y);

    // Start/end scanlines using top-left rule (center sampling, right/bottom exclusive).
    const bool rasterClip = cfg.cull.rasterClip;
    int yStart = static_cast<int>(std::ceil(v0.y - 0.5f));
    int yEnd = static_cast<int>(std::ceil(v2.y - 0.5f)) - 1;
    if (rasterClip) {
        yStart = std::max(0, yStart);
        yEnd = std::min(yEnd, static_cast<int>(framebuffer.height - 1));
    }
    // Initialize edge intersections at the first covered pixel center.
    const float yStartCenter = static_cast<float>(yStart) + 0.5f;
    float currentX1 = v0.x + static_cast<float>(invslope1) * (yStartCenter - v0.y);
    float currentX2 = v0.x + static_cast<float>(invslope2) * (yStartCenter - v0.y);
    float currentZ1 = v0.z + static_cast<float>(invslopez1) * (yStartCenter - v0.y);
    float currentZ2 = v0.z + static_cast<float>(invslopez2) * (yStartCenter - v0.y);

    for (int y = yStart; y <= yEnd; y++) {
        // TODO: add raster clip toggle

        // Raster clipping with top-left rule.
        const float minX = std::min(currentX1, currentX2);
        const float maxX = std::max(currentX1, currentX2);
        const float minZ = (currentX1 <= currentX2) ? currentZ1 : currentZ2;
        const float maxZ = (currentX1 <= currentX2) ? currentZ2 : currentZ1;
        int xStart = static_cast<int>(std::ceil(minX - 0.5f));
        int xEnd = static_cast<int>(std::ceil(maxX - 0.5f)) - 1;
        if (rasterClip) {
            xStart = std::max(0, xStart);
            xEnd = std::min(xEnd, static_cast<int>(framebuffer.width - 1));
        }

        const float denom = (xEnd - xStart) != 0 ? 1.0f / static_cast<float>(xEnd - xStart) : 0.0f;
        for (int x = xStart; x <= xEnd; x++) {
            const float t = (xEnd == xStart) ? 0.0f : (x - xStart) * denom;
            const float z = minZ + (maxZ - minZ) * t;
            // Depth test (lower z is closer).
            if (!cfg.cull.depthTest || z < depthBuffer.data[y * depthBuffer.width + x]) {
                if (cfg.cull.depthTest) {
                    depthBuffer.data[y * depthBuffer.width + x] = z;
                }
                DrawPixel(framebuffer, x, y, rasterClip, fillColor);
            }
        }
        currentX1 += invslope1;
        currentX2 += invslope2;
        currentZ1 += invslopez1;
        currentZ2 += invslopez2;
    }
}

/**
 * @brief Rasterizes a flat-top triangle (flat side: v1-v2)
 */
void Rasterizer::FillFlatTopTri(Buffer<Pixel>& framebuffer,
                                Buffer<float>& depthBuffer,
                                glm::vec3& v0,
                                glm::vec3& v1,
                                glm::vec3& v2,
                                const Config& cfg,
                                Pixel fillColor) {
    // Calculate invslopes in screen space
    // Run over rise, because edges can be completely vertical (infinite slope)
    double invslope1 = (v2.x - v0.x) / (v2.y - v0.y);
    double invslope2 = (v2.x - v1.x) / (v2.y - v1.y);
    double invslopez1 = (v2.z - v0.z) / (v2.y - v0.y);
    double invslopez2 = (v2.z - v1.z) / (v2.y - v1.y);

    // Start/end scanlines using top-left rule (center sampling, right/bottom exclusive).
    const bool rasterClip = cfg.cull.rasterClip;
    int yStart = static_cast<int>(std::ceil(v2.y - 0.5f)) - 1;
    int yEnd = static_cast<int>(std::ceil(v0.y - 0.5f));
    if (rasterClip) {
        yStart = std::min(yStart, static_cast<int>(framebuffer.height - 1));
        yEnd = std::max(0, yEnd);
    }
    // Initialize edge intersections at the first covered pixel center.
    const float yStartCenter = static_cast<float>(yStart) + 0.5f;
    float currentX1 = v2.x + static_cast<float>(invslope1) * (yStartCenter - v2.y);
    float currentX2 = v2.x + static_cast<float>(invslope2) * (yStartCenter - v2.y);
    float currentZ1 = v2.z + static_cast<float>(invslopez1) * (yStartCenter - v2.y);
    float currentZ2 = v2.z + static_cast<float>(invslopez2) * (yStartCenter - v2.y);

    for (int y = yStart; y >= yEnd; y--) {
        // TODO: add raster clip toggle

        // Raster clipping with top-left rule.
        const float minX = std::min(currentX1, currentX2);
        const float maxX = std::max(currentX1, currentX2);
        const float minZ = (currentX1 <= currentX2) ? currentZ1 : currentZ2;
        const float maxZ = (currentX1 <= currentX2) ? currentZ2 : currentZ1;
        int xStart = static_cast<int>(std::ceil(minX - 0.5f));
        int xEnd = static_cast<int>(std::ceil(maxX - 0.5f)) - 1;
        if (rasterClip) {
            xStart = std::max(0, xStart);
            xEnd = std::min(xEnd, static_cast<int>(framebuffer.width - 1));
        }

        const float denom = (xEnd - xStart) != 0 ? 1.0f / static_cast<float>(xEnd - xStart) : 0.0f;
        for (int x = xStart; x <= xEnd; x++) {
            const float t = (xEnd == xStart) ? 0.0f : (x - xStart) * denom;
            const float z = minZ + (maxZ - minZ) * t;
            if (!cfg.cull.depthTest || z < depthBuffer.data[y * depthBuffer.width + x]) {
                if (cfg.cull.depthTest) {
                    depthBuffer.data[y * depthBuffer.width + x] = z;
                }
                DrawPixel(framebuffer, x, y, rasterClip, fillColor);
            }
        }
        currentX1 -= invslope1;
        currentX2 -= invslope2;
        currentZ1 -= invslopez1;
        currentZ2 -= invslopez2;
    }
}

void Rasterizer::DrawPixel(Buffer<Pixel>& framebuffer, float x, float y, bool rasterClip, Pixel color) {
    if (rasterClip) {
        if (x < 0 || x >= framebuffer.width || y < 0 || y >= framebuffer.height) {
            return;
        }
    }
    framebuffer.Set(static_cast<int>(x), static_cast<int>(y), color);
}
} // namespace RetroRenderer
