#pragma once

#include <cstdint>

namespace RetroRenderer {

struct SceneMemoryStats {
    uint64_t geometryBytes = 0;
    uint64_t textureBytes = 0;

    [[nodiscard]] uint64_t TotalBytes() const {
        return geometryBytes + textureBytes;
    }
};

struct SoftwareRendererMemoryStats {
    uint64_t framebufferColorBytes = 0;
    uint64_t depthBufferBytes = 0;
    uint64_t scratchBytes = 0;
    uint64_t deferredTriangleBytes = 0;
    uint64_t skyboxFaceBytes = 0;
    uint64_t skyboxCacheBytes = 0;

    [[nodiscard]] uint64_t TotalBytes() const {
        return framebufferColorBytes + depthBufferBytes + scratchBytes + deferredTriangleBytes + skyboxFaceBytes + skyboxCacheBytes;
    }
};

struct HardwareRendererMemoryStats {
    uint64_t depthBufferBytes = 0;
    uint64_t meshCacheBytes = 0;
    uint64_t textureCacheBytes = 0;
    uint64_t skyboxBytes = 0;
    uint64_t fallbackTextureBytes = 0;

    [[nodiscard]] uint64_t TotalBytes() const {
        return depthBufferBytes + meshCacheBytes + textureCacheBytes + skyboxBytes + fallbackTextureBytes;
    }
};

} // namespace RetroRenderer
