#pragma once
#include <atomic>
#include <cstdint>

namespace RetroRenderer {
struct Stats {
    int renderedTris = 0;
    int renderedVerts = 0;
    std::atomic<uint64_t> swJobsSubmitted = 0;
    std::atomic<uint64_t> swJobsCompleted = 0;
    std::atomic<uint64_t> swJobsCancelled = 0;
    std::atomic<uint64_t> swJobsDroppedPending = 0;
    std::atomic<uint64_t> swFramesUploaded = 0;
    std::atomic<uint64_t> swFramesDroppedReady = 0;

    void Reset() {
        renderedTris = 0;
        renderedVerts = 0;
    }
};
} // namespace RetroRenderer
