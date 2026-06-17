#pragma once

#include "../GLBackendRendererBase.h"

namespace RetroRenderer {

class GLRenderer : public GLBackendRendererBase {
  public:
    GLRenderer() = default;
    ~GLRenderer() override = default;

    bool Init(TextureHandle fbTex, int w, int h) override;
    void DrawGridGizmo() override;

  protected:
    std::string_view GetShaderPrefix() const override;
    const char* GetRendererLogLabel() const override;
    void ConfigureBackendDebugState() override;
    void InitializeBackendResources() override;
    void DestroyBackendResources() override;
    void ApplyBackendFrameState() override;
    void ResetBackendFrameState() override;
    void RenderBackendOverlays() override;

  private:
    static void DebugCallback(GLenum source,
                              GLenum type,
                              GLuint id,
                              GLenum severity,
                              GLsizei length,
                              const GLchar* message,
                              const void* userParam);

    ShaderHandle m_GridProgram;
    GLuint m_GridVAO = 0;
    GLuint m_GridVBO = 0;
};

} // namespace RetroRenderer
