#pragma once

#include <cstdint>

namespace RetroRenderer {

struct ShaderHandle {
    uintptr_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const {
        return value != 0;
    }

    explicit constexpr operator bool() const {
        return IsValid();
    }
};

} // namespace RetroRenderer
