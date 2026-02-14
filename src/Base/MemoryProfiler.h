#pragma once

#include <cstdint>

namespace RetroRenderer {

struct ProcessMemorySnapshot {
    uint64_t residentBytes = 0;
    uint64_t peakResidentBytes = 0;
    bool supported = false;
};

class MemoryProfiler {
  public:
    static ProcessMemorySnapshot SampleProcessMemory();
};

} // namespace RetroRenderer
