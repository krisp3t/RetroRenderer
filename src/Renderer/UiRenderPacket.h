#pragma once

#include "TextureHandle.h"
#include <cstdint>
#include <imgui.h>
#include <vector>

namespace RetroRenderer {

struct UiDrawCommand {
    ImVec4 clipRect{};
    TextureHandle texture;
    uint32_t elementCount = 0;
    uint32_t indexOffset = 0;
    uint32_t vertexOffset = 0;
    bool resetRenderState = false;
};

struct UiDrawList {
    std::vector<ImDrawVert> vertices;
    std::vector<ImDrawIdx> indices;
    std::vector<UiDrawCommand> commands;
};

struct UiRenderPacket {
    ImVec2 displayPosition{};
    ImVec2 displaySize{};
    ImVec2 framebufferScale{1.0f, 1.0f};
    std::vector<UiDrawList> drawLists;
    bool valid = false;

    [[nodiscard]] static UiRenderPacket Capture(const ImDrawData* drawData);
};

} // namespace RetroRenderer
