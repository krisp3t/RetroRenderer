#include "UiRenderPacket.h"
#include <KrisLogger/Logger.h>
#include <type_traits>

namespace RetroRenderer {
namespace {

template <typename TextureId>
TextureHandle CaptureTextureIdImpl(TextureId textureId) {
    if constexpr (std::is_pointer_v<TextureId>) {
        return TextureHandle{reinterpret_cast<uintptr_t>(textureId)};
    } else {
        return TextureHandle{static_cast<uintptr_t>(textureId)};
    }
}

TextureHandle CaptureTextureId(ImTextureID textureId) {
    return CaptureTextureIdImpl(textureId);
}

} // namespace

UiRenderPacket UiRenderPacket::Capture(const ImDrawData* drawData) {
    UiRenderPacket packet{};
    if (drawData == nullptr || !drawData->Valid) {
        return packet;
    }

    packet.displayPosition = drawData->DisplayPos;
    packet.displaySize = drawData->DisplaySize;
    packet.framebufferScale = drawData->FramebufferScale;
    packet.drawLists.reserve(static_cast<size_t>(drawData->CmdListsCount));

    for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
        const ImDrawList* source = drawData->CmdLists[listIndex];
        UiDrawList list{};
        list.vertices.assign(source->VtxBuffer.Data, source->VtxBuffer.Data + source->VtxBuffer.Size);
        list.indices.assign(source->IdxBuffer.Data, source->IdxBuffer.Data + source->IdxBuffer.Size);
        list.commands.reserve(static_cast<size_t>(source->CmdBuffer.Size));

        for (const ImDrawCmd& sourceCommand : source->CmdBuffer) {
            if (sourceCommand.UserCallback != nullptr &&
                sourceCommand.UserCallback != ImDrawCallback_ResetRenderState) {
                LOGW("Skipping unsupported ImGui draw callback in detached UI packet");
                continue;
            }

            UiDrawCommand command{};
            command.clipRect = sourceCommand.ClipRect;
            command.texture = CaptureTextureId(sourceCommand.GetTexID());
            command.elementCount = sourceCommand.ElemCount;
            command.indexOffset = sourceCommand.IdxOffset;
            command.vertexOffset = sourceCommand.VtxOffset;
            command.resetRenderState = sourceCommand.UserCallback == ImDrawCallback_ResetRenderState;
            list.commands.push_back(command);
        }
        packet.drawLists.push_back(std::move(list));
    }

    packet.valid = true;
    return packet;
}

} // namespace RetroRenderer
