#pragma once

#include "CpuFrame.h"
#include "RenderPacket.h"
#include "UiRenderPacket.h"
#include <memory>
#include <vector>

namespace RetroRenderer {

struct UiFontAtlas {
    std::vector<Pixel> pixels;
    int width = 0;
    int height = 0;
};

namespace PresentationTexture {
inline constexpr TextureHandle Output{1};
inline constexpr TextureHandle MaterialPreview{2};
inline constexpr TextureHandle FontAtlas{3};
} // namespace PresentationTexture

struct UiTextureSnapshot {
    TextureHandle handle;
    std::shared_ptr<const Texture> texture;
    bool nearestNeighbor = false;
};

struct FrameSubmission {
    FrameSubmission() = default;
    FrameSubmission(const FrameSubmission&) = delete;
    FrameSubmission& operator=(const FrameSubmission&) = delete;
    FrameSubmission(FrameSubmission&&) noexcept = default;
    FrameSubmission& operator=(FrameSubmission&&) noexcept = default;

    uint64_t frameId = 0;
    std::shared_ptr<const RenderPacket> renderPacket;
    std::shared_ptr<const CpuFrame> softwareFrame;
    std::vector<UiTextureSnapshot> uiTextures;
    UiRenderPacket ui;
    bool enableVsync = true;
};

} // namespace RetroRenderer
