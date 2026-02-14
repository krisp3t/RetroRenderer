#include <catch2/catch_test_macros.hpp>

#include "Base/Stats.h"
#include "Base/Config.h"
#include "Renderer/Buffer.h"
#include "Renderer/Software/Rasterizer.h"
#include "Scene/ImportedSceneData.h"
#include "Scene/LightweightObjSceneImporter.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace RetroRenderer {
namespace {
bool PixelsEqual(const Pixel& lhs, const Pixel& rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

Config MakePipelineConfig() {
    Config config{};
    config.cull.backfaceCulling = false;
    config.cull.depthTest = true;
    config.cull.rasterClip = true;
    config.software.rasterizer.polygonMode = Config::RasterizationPolygonMode::FILL;
    config.software.rasterizer.fillMode = Config::RasterizationFillMode::BARYCENTRIC;
    return config;
}

bool ImportAndRasterize(const std::string& objText, uint64_t& outHash) {
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
    const Config config = MakePipelineConfig();
    const Pixel drawColor{60, 190, 250, 255};

    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const unsigned int i0 = mesh.indices[i + 0];
        const unsigned int i1 = mesh.indices[i + 1];
        const unsigned int i2 = mesh.indices[i + 2];
        if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size()) {
            return false;
        }
        std::array<Vertex, 3> tri = {mesh.vertices[i0], mesh.vertices[i1], mesh.vertices[i2]};
        Rasterizer::DrawTriangle(framebuffer, depthBuffer, tri, config, drawColor);
    }

    // FNV-1a hash over RGBA bytes
    uint64_t hash = 1469598103934665603ull;
    const uint64_t prime = 1099511628211ull;
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

    size_t colored = 0;
    for (size_t i = 0; i < pixelCount; i++) {
        if (PixelsEqual(framebuffer.data[i], drawColor)) {
            colored++;
        }
    }
    if (colored == 0) {
        return false;
    }

    outHash = hash;
    return true;
}
} // namespace

TEST_CASE("Lightweight OBJ importer can be used concurrently across threads", "[concurrency][importer]") {
    const std::string objText =
        "v -0.5 -0.5 0\n"
        "v 0.5 -0.5 0\n"
        "v 0.0 0.5 0\n"
        "f 1 2 3\n";

    constexpr int kThreads = 8;
    constexpr int kIterationsPerThread = 80;
    std::atomic<int> successfulLoads = 0;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; t++) {
        workers.emplace_back([&objText, &successfulLoads]() {
            LightweightObjSceneImporter importer;
            for (int i = 0; i < kIterationsPerThread; i++) {
                ImportedSceneData sceneData{};
                if (!importer.LoadFromMemory(reinterpret_cast<const uint8_t*>(objText.data()), objText.size(), sceneData)) {
                    continue;
                }
                if (sceneData.meshes.size() != 1) {
                    continue;
                }
                if (sceneData.meshes[0].vertices.size() != 3 || sceneData.meshes[0].indices.size() != 3) {
                    continue;
                }
                successfulLoads.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    REQUIRE(successfulLoads.load(std::memory_order_relaxed) == kThreads * kIterationsPerThread);
}

TEST_CASE("Importer+rasterizer pipeline stays deterministic under concurrent execution", "[concurrency][pipeline]") {
    const std::string objText =
        "v -0.7 -0.7 0\n"
        "v 0.7 -0.7 0\n"
        "v 0.0 0.7 0\n"
        "f 1 2 3\n";

    constexpr int kThreads = 8;
    std::vector<uint64_t> hashes(kThreads, 0);
    std::atomic<int> successfulRuns = 0;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; t++) {
        workers.emplace_back([t, &objText, &hashes, &successfulRuns]() {
            uint64_t hash = 0;
            if (ImportAndRasterize(objText, hash)) {
                hashes[t] = hash;
                successfulRuns.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    REQUIRE(successfulRuns.load(std::memory_order_relaxed) == kThreads);
    for (int i = 1; i < kThreads; i++) {
        REQUIRE(hashes[i] == hashes[0]);
    }
}

TEST_CASE("Software async stats atomics accumulate correctly under contention", "[concurrency][stats]") {
    Stats stats{};
    constexpr int kThreads = 8;
    constexpr int kIterations = 10000;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; t++) {
        workers.emplace_back([&stats]() {
            for (int i = 0; i < kIterations; i++) {
                stats.swJobsSubmitted.fetch_add(1, std::memory_order_relaxed);
                stats.swJobsCompleted.fetch_add(1, std::memory_order_relaxed);
                stats.swJobsCancelled.fetch_add(1, std::memory_order_relaxed);
                stats.swJobsDroppedPending.fetch_add(1, std::memory_order_relaxed);
                stats.swFramesUploaded.fetch_add(1, std::memory_order_relaxed);
                stats.swFramesDroppedReady.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (std::thread& worker : workers) {
        worker.join();
    }

    const uint64_t expected = static_cast<uint64_t>(kThreads) * static_cast<uint64_t>(kIterations);
    REQUIRE(stats.swJobsSubmitted.load(std::memory_order_relaxed) == expected);
    REQUIRE(stats.swJobsCompleted.load(std::memory_order_relaxed) == expected);
    REQUIRE(stats.swJobsCancelled.load(std::memory_order_relaxed) == expected);
    REQUIRE(stats.swJobsDroppedPending.load(std::memory_order_relaxed) == expected);
    REQUIRE(stats.swFramesUploaded.load(std::memory_order_relaxed) == expected);
    REQUIRE(stats.swFramesDroppedReady.load(std::memory_order_relaxed) == expected);
}

} // namespace RetroRenderer
