#pragma once

#include "../GLBackendRendererBase.h"

namespace RetroRenderer {

class GLESRenderer : public GLBackendRendererBase {
  public:
    GLESRenderer() = default;
    ~GLESRenderer() override = default;

    bool Init(TextureHandle fbTex, int w, int h) override;

  protected:
    std::string_view GetShaderPrefix() const override;
    const char* GetRendererLogLabel() const override;
};

} // namespace RetroRenderer
