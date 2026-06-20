#pragma once
#include <atomic>
#include <cstdint>

namespace RetroRenderer {
struct Stats {
    int renderedTris = 0;
    int renderedVerts = 0;
    bool processMemorySupported = false;
    uint64_t processResidentBytes = 0;
    uint64_t processResidentPeakBytes = 0;
    uint64_t processResidentPeakOsBytes = 0;
    uint64_t sceneGeometryCpuBytes = 0;
    uint64_t sceneTextureCpuBytes = 0;
    uint64_t softwareFramebufferColorBytes = 0;
    uint64_t softwareDepthBufferBytes = 0;
    uint64_t softwareScratchBytes = 0;
    uint64_t softwareDeferredTriangleBytes = 0;
    uint64_t softwareSkyboxBytes = 0;
    uint64_t softwareReadyFrameBytes = 0;
    uint64_t softwarePresentedFrameBytes = 0;
    uint64_t glRendererDepthBufferBytes = 0;
    uint64_t glRendererMeshCacheBytes = 0;
    uint64_t glRendererTextureCacheBytes = 0;
    uint64_t glRendererSkyboxBytes = 0;
    uint64_t glRendererFallbackTextureBytes = 0;
    uint64_t outputPresenterBytes = 0;
    uint64_t previewPresenterBytes = 0;
    uint64_t fontPresenterBytes = 0;
    std::atomic<uint64_t> swJobsSubmitted = 0;
    std::atomic<uint64_t> swJobsCompleted = 0;
    std::atomic<uint64_t> swJobsCancelled = 0;
    std::atomic<uint64_t> swJobsReplacedPending = 0;
    std::atomic<uint64_t> swFramesPresented = 0;
    std::atomic<uint64_t> swFramesReplacedReady = 0;
    std::atomic<uint64_t> lastSoftwareFramePresentedNs = 0;
    std::atomic<uint64_t> lastSoftwareFramePresentIntervalNs = 0;
    std::atomic<uint64_t> lastFrameTotalNs = 0;
    std::atomic<uint64_t> lastMainUpdateNs = 0;
    std::atomic<uint64_t> lastDisplayBeforeFrameNs = 0;
    std::atomic<uint64_t> lastRenderPacketBuildNs = 0;
    std::atomic<uint64_t> lastRenderSystemNs = 0;
    std::atomic<uint64_t> lastGlRenderNs = 0;
    std::atomic<uint64_t> lastSoftwarePacketCopyNs = 0;
    std::atomic<uint64_t> lastSoftwareWorkerRenderNs = 0;
    std::atomic<uint64_t> lastSoftwareWorkerCopyNs = 0;
    std::atomic<uint64_t> lastDisplayDrawNs = 0;
    std::atomic<uint64_t> lastCpuOutputUploadNs = 0;
    std::atomic<uint64_t> lastImGuiBuildNs = 0;
    std::atomic<uint64_t> lastImGuiRenderNs = 0;
    std::atomic<uint64_t> lastSwapBuffersNs = 0;

    void Reset() {
        renderedTris = 0;
        renderedVerts = 0;
    }

    void UpdateProcessMemory(uint64_t residentBytes, uint64_t osPeakResidentBytes) {
        processMemorySupported = true;
        processResidentBytes = residentBytes;
        if (processResidentBytes > processResidentPeakBytes) {
            processResidentPeakBytes = processResidentBytes;
        }
        if (osPeakResidentBytes > processResidentPeakOsBytes) {
            processResidentPeakOsBytes = osPeakResidentBytes;
        }
    }

    [[nodiscard]] uint64_t KnownResidentBytes() const {
        return sceneGeometryCpuBytes +
               sceneTextureCpuBytes +
               softwareFramebufferColorBytes +
               softwareDepthBufferBytes +
               softwareScratchBytes +
               softwareDeferredTriangleBytes +
               softwareSkyboxBytes +
               softwareReadyFrameBytes +
               softwarePresentedFrameBytes +
               glRendererDepthBufferBytes +
               glRendererMeshCacheBytes +
               glRendererTextureCacheBytes +
               glRendererSkyboxBytes +
               glRendererFallbackTextureBytes +
               outputPresenterBytes +
               previewPresenterBytes +
               fontPresenterBytes;
    }
};
} // namespace RetroRenderer
