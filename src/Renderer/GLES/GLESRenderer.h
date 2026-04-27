#pragma once
#include "../../Base/Color.h"
#include "../../Scene/Camera.h"
#include "../../Scene/MaterialManager.h"
#include "../../Scene/Scene.h"
#include "../../include/kris_glheaders.h"
#include "../GLMeshResourceCache.h"
#include "../GLTextureResourceCache.h"
#include "../IHardwareRenderer.h"

namespace RetroRenderer {
class GLESRenderer : public IHardwareRenderer {
  public:
    GLESRenderer() = default;

    ~GLESRenderer() = default;

    bool Init(TextureHandle fbTex, int w, int h) override;

    void Resize(TextureHandle newFbTex, int w, int h) override;

    void Destroy() override;
    void InvalidateSceneResources() override;
    void InvalidateTextureResources() override;

    void SetActiveCamera(const Camera& camera) override;
    void SetSceneLights(const std::vector<LightSnapshot>& lights) override;

    void DrawTriangularMesh(const Model* model) override;

    void DrawSkybox() override;

    void BeforeFrame(const Color& clearColor) override;

    void EndFrame() override;

    GLuint CreateShaderProgram();
    ShaderHandle CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) override;

  private:
    bool CreateFramebuffer(TextureHandle fbTex, int w, int h);
    GLuint CompileShader(GLenum shaderType, const char* shaderSource);
    void CheckShaderErrors(GLuint shader, const std::string& type);
    void CreateFallbackTexture();
    GLuint CreateCubemap(const std::string& path);
    GLuint CreateSkyboxVAO();
    void DestroyFramebufferResources();
    void DestroyRendererResources();

  private:
    Camera* p_Camera = nullptr;
    std::vector<LightSnapshot> m_SceneLights;

    GLuint p_FrameBufferTexture = 0;
    GLuint m_FrameBuffer = 0;
    GLuint m_DepthBuffer = 0;
    GLuint m_FallbackTexture = 0;
    GLuint m_SkyboxTexture = 0;
    GLuint m_SkyboxVAO = 0;
    GLuint m_SkyboxVBO = 0;
    int m_FrameWidth = 0;
    int m_FrameHeight = 0;
    ShaderProgram m_SkyboxProgram;
    GLMeshResourceCache m_MeshResources;
    GLTextureResourceCache m_TextureResources;
};
} // namespace RetroRenderer
