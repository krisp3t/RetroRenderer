#pragma once

#include <cstdint>

namespace RetroRenderer {

enum class RenderOutputKind {
    None,
    TextureHandle,
};

enum class RenderOutputOrigin {
    TopLeft,
    BottomLeft,
};

struct RenderOutput {
    RenderOutputKind kind = RenderOutputKind::None;
    uintptr_t handle = 0;
    RenderOutputOrigin origin = RenderOutputOrigin::TopLeft;

    [[nodiscard]] static constexpr RenderOutput Texture(uintptr_t textureHandle, RenderOutputOrigin textureOrigin) {
        return RenderOutput{RenderOutputKind::TextureHandle, textureHandle, textureOrigin};
    }

    [[nodiscard]] constexpr bool IsValid() const {
        return kind != RenderOutputKind::None && handle != 0;
    }
};

} // namespace RetroRenderer
