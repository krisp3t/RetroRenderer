#pragma once

#include "IRenderer.h"
#include "TextureHandle.h"

namespace RetroRenderer {

class IHardwareRenderer : public IRenderer {
  public:
    IHardwareRenderer() = default;
    ~IHardwareRenderer() override = default;

    virtual bool Init(TextureHandle framebufferTexture, int width, int height) = 0;
    virtual void Resize(TextureHandle framebufferTexture, int width, int height) = 0;
    virtual void InvalidateSceneResources() = 0;
    virtual void InvalidateTextureResources() = 0;
};

} // namespace RetroRenderer
