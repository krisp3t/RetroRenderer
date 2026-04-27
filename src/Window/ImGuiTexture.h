#pragma once

#include "../Renderer/TextureHandle.h"
#include <imgui.h>
#include <type_traits>

namespace RetroRenderer {

namespace Detail {
template <typename TextureId>
TextureId ToImTextureID(TextureHandle handle) {
    if constexpr (std::is_pointer_v<TextureId>) {
        return reinterpret_cast<TextureId>(handle.value);
    } else {
        return static_cast<TextureId>(handle.value);
    }
}
} // namespace Detail

inline ImTextureID ToImTextureID(TextureHandle handle) {
    return Detail::ToImTextureID<ImTextureID>(handle);
}

} // namespace RetroRenderer
