#pragma once

#include "../Base/Color.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace RetroRenderer {

struct CpuFrame {
    std::vector<Pixel> pixels;
    size_t width = 0;
    size_t height = 0;
    size_t pitch = 0;
    uint64_t frameId = 0;
    uint64_t dataRevision = 0;

    [[nodiscard]] uint64_t EstimateResidentMemory() const {
        return sizeof(CpuFrame) + pixels.capacity() * sizeof(Pixel);
    }
};

} // namespace RetroRenderer
