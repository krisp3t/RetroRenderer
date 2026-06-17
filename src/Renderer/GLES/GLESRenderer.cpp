#include "GLESRenderer.h"

namespace RetroRenderer {

bool GLESRenderer::Init(TextureHandle fbTex, int w, int h) {
#ifdef __ANDROID__
    return InitializeBackend(fbTex, w, h, "img/skybox-cubemap/Cubemap_Sky_23-512x512.png");
#else
    return InitializeBackend(fbTex, w, h, "assets/img/skybox-cubemap/Cubemap_Sky_23-512x512.png");
#endif
}

std::string_view GLESRenderer::GetShaderPrefix() const {
    return "#version 300 es\nprecision highp float;\n";
}

const char* GLESRenderer::GetRendererLogLabel() const {
    return "GLESRenderer";
}

} // namespace RetroRenderer
