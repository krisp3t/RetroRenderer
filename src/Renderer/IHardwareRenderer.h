#pragma once

#include "IRenderer.h"
#include "TextureHandle.h"

namespace RetroRenderer {

class Texture;

class IHardwareRenderer : public IRenderer {
  public:
    IHardwareRenderer() = default;
    ~IHardwareRenderer() override = default;

    virtual bool Init(TextureHandle framebufferTexture, int width, int height) = 0;
    virtual void Resize(TextureHandle framebufferTexture, int width, int height) = 0;
    virtual void InvalidateSceneResources() = 0;
    virtual void InvalidateTextureResources() = 0;
    [[nodiscard]] virtual TextureHandle GetTextureHandle(const Texture& texture) = 0;
};

} // namespace RetroRenderer
