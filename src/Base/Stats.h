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
};
} // namespace RetroRenderer
