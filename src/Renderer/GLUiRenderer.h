#pragma once

#include "UiRenderPacket.h"
#include "../include/kris_glheaders.h"
#include <functional>

namespace RetroRenderer {

class GLUiRenderer {
  public:
    using TextureResolver = std::function<GLuint(TextureHandle)>;

    bool Init();
    void Render(const UiRenderPacket& packet, const TextureResolver& resolveTexture);
    void Destroy();

  private:
    bool CreateDeviceObjects();
    void SetupRenderState(const UiRenderPacket& packet, int framebufferWidth, int framebufferHeight);

    GLuint m_ShaderProgram = 0;
    GLuint m_VertexShader = 0;
    GLuint m_FragmentShader = 0;
    GLuint m_VertexArray = 0;
    GLuint m_VertexBuffer = 0;
    GLuint m_IndexBuffer = 0;
    GLint m_TextureLocation = -1;
    GLint m_ProjectionLocation = -1;
};

} // namespace RetroRenderer
