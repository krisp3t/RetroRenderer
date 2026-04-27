#pragma once

#include "TextureHandle.h"

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
    TextureHandle textureHandle;
    RenderOutputOrigin origin = RenderOutputOrigin::TopLeft;

    [[nodiscard]] static constexpr RenderOutput Texture(TextureHandle textureHandle, RenderOutputOrigin textureOrigin) {
        return RenderOutput{RenderOutputKind::TextureHandle, textureHandle, textureOrigin};
    }

    [[nodiscard]] constexpr bool IsValid() const {
        return kind != RenderOutputKind::None && textureHandle.IsValid();
    }
};

} // namespace RetroRenderer
