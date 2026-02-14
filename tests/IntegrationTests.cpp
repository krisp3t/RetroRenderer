#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Base/Config.h"
#include "Renderer/Buffer.h"
#include "Renderer/Software/Rasterizer.h"
#include "Scene/ImportedSceneData.h"
#include "Scene/LightweightObjSceneImporter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

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

Config MakePipelineConfig(bool depthTest, bool rasterClip) {
    Config config{};
    config.cull.backfaceCulling = false;
    config.cull.depthTest = depthTest;
    config.cull.rasterClip = rasterClip;
    config.software.rasterizer.polygonMode = Config::RasterizationPolygonMode::FILL;
    config.software.rasterizer.fillMode = Config::RasterizationFillMode::BARYCENTRIC;
    return config;
}

ImportedSceneData ImportObj(const std::string& objText) {
    LightweightObjSceneImporter importer;
    ImportedSceneData sceneData{};
    const bool ok = importer.LoadFromMemory(reinterpret_cast<const uint8_t*>(objText.data()), objText.size(), sceneData);
    REQUIRE(ok);
    REQUIRE(sceneData.meshes.size() == 1);
    return sceneData;
}

void DrawFace(const ImportedMesh& mesh,
              size_t baseIndex,
              Buffer<Pixel>& framebuffer,
              Buffer<float>& depthBuffer,
              const Config& config,
              Pixel color) {
    REQUIRE(baseIndex + 2 < mesh.indices.size());
    const unsigned int i0 = mesh.indices[baseIndex + 0];
    const unsigned int i1 = mesh.indices[baseIndex + 1];
    const unsigned int i2 = mesh.indices[baseIndex + 2];
    REQUIRE(i0 < mesh.vertices.size());
    REQUIRE(i1 < mesh.vertices.size());
    REQUIRE(i2 < mesh.vertices.size());

    std::array<Vertex, 3> triangle = {mesh.vertices[i0], mesh.vertices[i1], mesh.vertices[i2]};
    Rasterizer::DrawTriangle(framebuffer, depthBuffer, triangle, config, color);
}
} // namespace

TEST_CASE("OBJ importer + rasterizer pipeline renders triangle into framebuffer", "[integration][pipeline]") {
    const std::string objText =
        "v -0.7 -0.7 0.0\n"
        "v 0.7 -0.7 0.0\n"
        "v 0.0 0.7 0.0\n"
        "f 1 2 3\n";

    ImportedSceneData sceneData = ImportObj(objText);
    const ImportedMesh& mesh = sceneData.meshes[0];
    REQUIRE(mesh.indices.size() == 3);

    Buffer<Pixel> framebuffer(64, 64);
    Buffer<float> depthBuffer(64, 64);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);

    const Pixel drawColor{180, 220, 50, 255};
    const Config config = MakePipelineConfig(true, true);
    DrawFace(mesh, 0, framebuffer, depthBuffer, config, drawColor);

    REQUIRE(CountPixels(framebuffer, drawColor) > 0);
    const size_t centerIndex = (framebuffer.height / 2) * framebuffer.width + (framebuffer.width / 2);
    REQUIRE(PixelsEqual(framebuffer.data[centerIndex], drawColor));
    REQUIRE(depthBuffer.data[centerIndex] < 1.0f);
}

TEST_CASE("OBJ importer + rasterizer depth pipeline keeps nearest face in overlap", "[integration][pipeline][depth]") {
    const std::string objText =
        "v -0.6 -0.6 -0.8\n"
        "v 0.6 -0.6 -0.8\n"
        "v 0.0 0.6 -0.8\n"
        "v -0.6 -0.6 0.8\n"
        "v 0.6 -0.6 0.8\n"
        "v 0.0 0.6 0.8\n"
        "f 1 2 3\n"
        "f 4 5 6\n";

    ImportedSceneData sceneData = ImportObj(objText);
    const ImportedMesh& mesh = sceneData.meshes[0];
    REQUIRE(mesh.indices.size() == 6);

    Buffer<Pixel> framebuffer(64, 64);
    Buffer<float> depthBuffer(64, 64);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);

    const Pixel nearColor{255, 60, 60, 255};
    const Pixel farColor{60, 60, 255, 255};
    const Config config = MakePipelineConfig(true, true);

    DrawFace(mesh, 0, framebuffer, depthBuffer, config, nearColor);
    DrawFace(mesh, 3, framebuffer, depthBuffer, config, farColor);

    const size_t centerIndex = (framebuffer.height / 2) * framebuffer.width + (framebuffer.width / 2);
    REQUIRE(PixelsEqual(framebuffer.data[centerIndex], nearColor));
    REQUIRE(depthBuffer.data[centerIndex] == Catch::Approx(0.1f).margin(0.02f));
    REQUIRE(CountPixels(framebuffer, farColor) == 0);
}

TEST_CASE("OBJ importer + rasterizer clipping pipeline ignores fully out-of-bounds face", "[integration][pipeline][clipping]") {
    const std::string objText =
        "v 1.2 -0.2 0.0\n"
        "v 1.6 -0.2 0.0\n"
        "v 1.4 0.6 0.0\n"
        "v -0.6 -0.6 0.0\n"
        "v 0.6 -0.6 0.0\n"
        "v 0.0 0.6 0.0\n"
        "f 1 2 3\n"
        "f 4 5 6\n";

    ImportedSceneData sceneData = ImportObj(objText);
    const ImportedMesh& mesh = sceneData.meshes[0];
    REQUIRE(mesh.indices.size() == 6);

    Buffer<Pixel> framebuffer(64, 64);
    Buffer<float> depthBuffer(64, 64);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);

    const Pixel clippedOutColor{255, 120, 0, 255};
    const Pixel visibleColor{0, 220, 140, 255};
    const Config config = MakePipelineConfig(false, true);

    DrawFace(mesh, 0, framebuffer, depthBuffer, config, clippedOutColor);
    DrawFace(mesh, 3, framebuffer, depthBuffer, config, visibleColor);

    REQUIRE(CountPixels(framebuffer, clippedOutColor) == 0);
    REQUIRE(CountPixels(framebuffer, visibleColor) > 0);
}

} // namespace RetroRenderer
