#pragma once

#include <cstdint>

namespace RetroRenderer {

struct TextureHandle {
    uintptr_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const {
        return value != 0;
    }

    explicit constexpr operator bool() const {
        return IsValid();
    }
};

} // namespace RetroRenderer
