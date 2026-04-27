#pragma once

#include "TextureHandle.h"
#include <cstddef>

namespace RetroRenderer {

struct Pixel;

enum class RenderOutputKind {
    None,
    TextureHandle,
    CpuPixels,
};

enum class RenderOutputOrigin {
    TopLeft,
    BottomLeft,
};

enum class RenderOutputPixelFormat {
    Rgba8,
};

struct RenderOutputCpuPixels {
    const Pixel* pixels = nullptr;
    size_t width = 0;
    size_t height = 0;
    size_t pitch = 0;
    RenderOutputPixelFormat format = RenderOutputPixelFormat::Rgba8;
};

struct RenderOutput {
    RenderOutputKind kind = RenderOutputKind::None;
    TextureHandle textureHandle;
    RenderOutputCpuPixels cpuPixels;
    RenderOutputOrigin origin = RenderOutputOrigin::TopLeft;

    [[nodiscard]] static constexpr RenderOutput Texture(TextureHandle textureHandle, RenderOutputOrigin textureOrigin) {
        return RenderOutput{RenderOutputKind::TextureHandle, textureHandle, {}, textureOrigin};
    }

    [[nodiscard]] static constexpr RenderOutput Pixels(const Pixel* pixels,
                                                       size_t width,
                                                       size_t height,
                                                       size_t pitch,
                                                       RenderOutputOrigin pixelOrigin,
                                                       RenderOutputPixelFormat format = RenderOutputPixelFormat::Rgba8) {
        return RenderOutput{RenderOutputKind::CpuPixels, {}, {pixels, width, height, pitch, format}, pixelOrigin};
    }

    [[nodiscard]] constexpr bool IsValid() const {
        switch (kind) {
        case RenderOutputKind::TextureHandle:
            return textureHandle.IsValid();
        case RenderOutputKind::CpuPixels:
            return cpuPixels.pixels != nullptr && cpuPixels.width > 0 && cpuPixels.height > 0 && cpuPixels.pitch > 0;
        case RenderOutputKind::None:
            return false;
        }
        return false;
    }
};

} // namespace RetroRenderer
