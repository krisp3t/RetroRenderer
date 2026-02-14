#include <catch2/catch_test_macros.hpp>

#include "Base/Config.h"
#include "GoldenHashUtils.h"
#include "Renderer/Buffer.h"
#include "Renderer/Software/Rasterizer.h"
#include "Scene/ImportedSceneData.h"
#include "Scene/LightweightObjSceneImporter.h"

#include <array>
#include <cstdlib>
#include <cstdint>
#include <string>

namespace RetroRenderer {
namespace {
constexpr const char* kGoldenHashFile = RETRO_GOLDEN_HASH_FILE;

std::string CurrentPlatformTag() {
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

std::string PlatformKey(const std::string& baseKey) {
    return baseKey + "." + CurrentPlatformTag();
}

bool IsTruthyEnv(const char* name) {
    const char* env = std::getenv(name);
    if (!env) {
        return false;
    }
    const std::string value = TestGolden::Trim(std::string(env));
    return value == "1" || value == "true" || value == "TRUE" || value == "on" || value == "ON";
}

bool ShouldRunGoldensOnCurrentPlatform() {
#if defined(_WIN32)
    return IsTruthyEnv("RETRO_RUN_GOLDEN_WINDOWS");
#else
    return true;
#endif
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

uint64_t HashFramebuffer(const Buffer<Pixel>& framebuffer) {
    uint64_t hash = 1469598103934665603ull;
    constexpr uint64_t prime = 1099511628211ull;
    const size_t pixelCount = framebuffer.width * framebuffer.height;
    for (size_t i = 0; i < pixelCount; i++) {
        const Pixel& p = framebuffer.data[i];
        hash ^= static_cast<uint64_t>(p.r);
        hash *= prime;
        hash ^= static_cast<uint64_t>(p.g);
        hash *= prime;
        hash ^= static_cast<uint64_t>(p.b);
        hash *= prime;
        hash ^= static_cast<uint64_t>(p.a);
        hash *= prime;
    }
    return hash;
}

bool RenderScenarioToHash(const std::string& objText,
                          bool depthTest,
                          bool rasterClip,
                          const std::array<Pixel, 4>& faceColors,
                          uint64_t& outHash) {
    LightweightObjSceneImporter importer;
    ImportedSceneData sceneData{};
    if (!importer.LoadFromMemory(reinterpret_cast<const uint8_t*>(objText.data()), objText.size(), sceneData)) {
        return false;
    }
    if (sceneData.meshes.size() != 1) {
        return false;
    }
    const ImportedMesh& mesh = sceneData.meshes[0];
    if (mesh.indices.size() % 3 != 0) {
        return false;
    }

    Buffer<Pixel> framebuffer(64, 64);
    Buffer<float> depthBuffer(64, 64);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);
    const Config config = MakePipelineConfig(depthTest, rasterClip);

    size_t face = 0;
    for (size_t i = 0; i < mesh.indices.size(); i += 3, face++) {
        const unsigned int i0 = mesh.indices[i + 0];
        const unsigned int i1 = mesh.indices[i + 1];
        const unsigned int i2 = mesh.indices[i + 2];
        if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size()) {
            return false;
        }
        std::array<Vertex, 3> triangle = {mesh.vertices[i0], mesh.vertices[i1], mesh.vertices[i2]};
        Rasterizer::DrawTriangle(framebuffer, depthBuffer, triangle, config, faceColors[face % faceColors.size()]);
    }

    outHash = HashFramebuffer(framebuffer);
    return true;
}

void AssertGoldenHash(const std::string& key, uint64_t actualHash) {
    auto hashes = TestGolden::LoadGoldenHashes(kGoldenHashFile);
    const std::string platformKey = PlatformKey(key);
    if (TestGolden::ShouldUpdateGoldens()) {
        hashes[platformKey] = actualHash;
        REQUIRE(TestGolden::SaveGoldenHashes(kGoldenHashFile, hashes));
        hashes = TestGolden::LoadGoldenHashes(kGoldenHashFile);
    }

    INFO("Golden hash file: " << kGoldenHashFile);
    INFO("Golden key: " << key);
    INFO("Platform key: " << platformKey);
    auto it = hashes.find(platformKey);
    if (it == hashes.end()) {
        it = hashes.find(key); // Backward-compatible fallback to platform-agnostic baseline.
    }
    REQUIRE(it != hashes.end());
    REQUIRE(it->second == actualHash);
}
} // namespace

TEST_CASE("Golden software pipeline hash: simple triangle", "[golden][software]") {
    if (!ShouldRunGoldensOnCurrentPlatform()) {
        SKIP("Set RETRO_RUN_GOLDEN_WINDOWS=1 to enforce golden hashes on Windows.");
    }
    const std::string objText =
        "v -0.7 -0.7 0.0\n"
        "v 0.7 -0.7 0.0\n"
        "v 0.0 0.7 0.0\n"
        "f 1 2 3\n";

    uint64_t actualHash = 0;
    REQUIRE(RenderScenarioToHash(objText,
                                 true,
                                 true,
                                 {Pixel{180, 220, 50, 255}, Pixel{0, 0, 0, 0}, Pixel{0, 0, 0, 0}, Pixel{0, 0, 0, 0}},
                                 actualHash));
    AssertGoldenHash("integration_triangle", actualHash);
}

TEST_CASE("Golden software pipeline hash: depth overlap", "[golden][software]") {
    if (!ShouldRunGoldensOnCurrentPlatform()) {
        SKIP("Set RETRO_RUN_GOLDEN_WINDOWS=1 to enforce golden hashes on Windows.");
    }
    const std::string objText =
        "v -0.6 -0.6 -0.8\n"
        "v 0.6 -0.6 -0.8\n"
        "v 0.0 0.6 -0.8\n"
        "v -0.6 -0.6 0.8\n"
        "v 0.6 -0.6 0.8\n"
        "v 0.0 0.6 0.8\n"
        "f 1 2 3\n"
        "f 4 5 6\n";

    uint64_t actualHash = 0;
    REQUIRE(RenderScenarioToHash(objText,
                                 true,
                                 true,
                                 {Pixel{255, 60, 60, 255}, Pixel{60, 60, 255, 255}, Pixel{0, 0, 0, 0}, Pixel{0, 0, 0, 0}},
                                 actualHash));
    AssertGoldenHash("integration_depth", actualHash);
}

TEST_CASE("Golden software pipeline hash: clipping", "[golden][software]") {
    if (!ShouldRunGoldensOnCurrentPlatform()) {
        SKIP("Set RETRO_RUN_GOLDEN_WINDOWS=1 to enforce golden hashes on Windows.");
    }
    const std::string objText =
        "v 1.2 -0.2 0.0\n"
        "v 1.6 -0.2 0.0\n"
        "v 1.4 0.6 0.0\n"
        "v -0.6 -0.6 0.0\n"
        "v 0.6 -0.6 0.0\n"
        "v 0.0 0.6 0.0\n"
        "f 1 2 3\n"
        "f 4 5 6\n";

    uint64_t actualHash = 0;
    REQUIRE(RenderScenarioToHash(objText,
                                 false,
                                 true,
                                 {Pixel{255, 120, 0, 255}, Pixel{0, 220, 140, 255}, Pixel{0, 0, 0, 0}, Pixel{0, 0, 0, 0}},
                                 actualHash));
    AssertGoldenHash("integration_clipping", actualHash);
}

} // namespace RetroRenderer
