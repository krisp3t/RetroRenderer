#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Base/Config.h"
#include "Renderer/Buffer.h"
#include "Renderer/Software/Rasterizer.h"
#include "Scene/Vertex.h"

#include <array>
#include <cstddef>

namespace RetroRenderer {
namespace {
bool PixelsEqual(const Pixel& lhs, const Pixel& rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

size_t CountPixels(const Buffer<Pixel>& framebuffer, const Pixel& color) {
    size_t count = 0;
    const size_t pixelCount = framebuffer.width * framebuffer.height;
    for (size_t i = 0; i < pixelCount; i++) {
        if (PixelsEqual(framebuffer.data[i], color)) {
            count++;
        }
    }
    return count;
}

Vertex MakeVertex(float x, float y, float z) {
    Vertex vertex{};
    vertex.position = glm::vec4(x, y, z, 1.0f);
    vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertex.texCoords = glm::vec2(0.0f);
    vertex.color = glm::vec3(1.0f);
    return vertex;
}

std::array<Vertex, 3> MakeLargeTriangle(float ndcZ) {
    return {MakeVertex(-0.8f, -0.8f, ndcZ), MakeVertex(0.8f, -0.8f, ndcZ), MakeVertex(0.0f, 0.8f, ndcZ)};
}

Config MakeBarycentricFillConfig() {
    Config config{};
    config.cull.backfaceCulling = false;
    config.cull.depthTest = true;
    config.cull.rasterClip = true;
    config.software.rasterizer.polygonMode = Config::RasterizationPolygonMode::FILL;
    config.software.rasterizer.fillMode = Config::RasterizationFillMode::BARYCENTRIC;
    return config;
}
} // namespace

TEST_CASE("NDCToViewport maps canonical coordinates with subpixel offset", "[rasterizer]") {
    const glm::vec2 topLeft = Rasterizer::NDCToViewport(glm::vec2(-1.0f, 1.0f), 640, 480);
    REQUIRE(topLeft.x == Catch::Approx(0.5f));
    REQUIRE(topLeft.y == Catch::Approx(0.5f));

    const glm::vec2 center = Rasterizer::NDCToViewport(glm::vec2(0.0f, 0.0f), 640, 480);
    REQUIRE(center.x == Catch::Approx(320.5f));
    REQUIRE(center.y == Catch::Approx(240.5f));
}

TEST_CASE("DrawTriangle fills framebuffer in barycentric mode", "[rasterizer]") {
    Buffer<Pixel> framebuffer(32, 32);
    Buffer<float> depthBuffer(32, 32);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);

    Config config = MakeBarycentricFillConfig();
    std::array<Vertex, 3> triangle = MakeLargeTriangle(0.0f);
    const Pixel fillColor{17, 34, 51, 255};

    Rasterizer::DrawTriangle(framebuffer, depthBuffer, triangle, config, fillColor);

    REQUIRE(CountPixels(framebuffer, fillColor) > 0);

    const size_t centerIndex = (framebuffer.height / 2) * framebuffer.width + (framebuffer.width / 2);
    REQUIRE(PixelsEqual(framebuffer.data[centerIndex], fillColor));
    REQUIRE(depthBuffer.data[centerIndex] < 1.0f);
}

TEST_CASE("Depth testing preserves the nearest triangle", "[rasterizer]") {
    Buffer<Pixel> framebuffer(32, 32);
    Buffer<float> depthBuffer(32, 32);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);

    Config config = MakeBarycentricFillConfig();
    std::array<Vertex, 3> nearTriangle = MakeLargeTriangle(-0.5f);
    std::array<Vertex, 3> farTriangle = MakeLargeTriangle(0.8f);
    const Pixel nearColor{255, 0, 0, 255};
    const Pixel farColor{0, 0, 255, 255};

    Rasterizer::DrawTriangle(framebuffer, depthBuffer, nearTriangle, config, nearColor);
    Rasterizer::DrawTriangle(framebuffer, depthBuffer, farTriangle, config, farColor);

    const size_t centerIndex = (framebuffer.height / 2) * framebuffer.width + (framebuffer.width / 2);
    REQUIRE(PixelsEqual(framebuffer.data[centerIndex], nearColor));
    REQUIRE(depthBuffer.data[centerIndex] == Catch::Approx(0.25f).margin(0.02f));
    REQUIRE(CountPixels(framebuffer, farColor) == 0);
}

} // namespace RetroRenderer
