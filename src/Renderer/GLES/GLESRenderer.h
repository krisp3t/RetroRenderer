#pragma once
#include "../../Base/Color.h"
#include "../../Scene/Camera.h"
#include "../../Scene/MaterialManager.h"
#include "../../Scene/Scene.h"
#include "../../include/kris_glheaders.h"
#include "../GLMeshResourceCache.h"
#include "../GLTextureResourceCache.h"
#include "../IRenderer.h"

namespace RetroRenderer {
class GLESRenderer : public IRenderer {
  public:
    GLESRenderer() = default;

    ~GLESRenderer() = default;

    bool Init(GLuint fbTex, int w, int h);

    void Resize(GLuint newFbTex, int w, int h);

    void Destroy() override;
    void InvalidateSceneResources();
    void InvalidateTextureResources();
    GLuint GetTextureHandle(const Texture& texture);

    void SetActiveCamera(const Camera& camera) override;
    void SetSceneLights(const std::vector<LightSnapshot>& lights) override;

    void DrawTriangularMesh(const Model* model) override;

    void DrawSkybox() override;

    void BeforeFrame(const Color& clearColor) override;

    void EndFrame() override;

    GLuint CreateShaderProgram();
    GLuint CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) override;

  private:
    bool CreateFramebuffer(GLuint fbTex, int w, int h);
    GLuint CompileShader(GLenum shaderType, const char* shaderSource);
    void CheckShaderErrors(GLuint shader, const std::string& type);
    void CreateFallbackTexture();
    GLuint CreateCubemap(const std::string& path);
    GLuint CreateSkyboxVAO();

  private:
    Camera* p_Camera = nullptr;
    std::vector<LightSnapshot> m_SceneLights;

    GLuint p_FrameBufferTexture = 0;
    GLuint m_FrameBuffer = 0;
    GLuint m_DepthBuffer = 0;
    GLuint m_FallbackTexture = 0;
    GLuint m_SkyboxTexture = 0;
    GLuint m_SkyboxVAO = 0;
    ShaderProgram m_SkyboxProgram;
    GLMeshResourceCache m_MeshResources;
    GLTextureResourceCache m_TextureResources;
};
} // namespace RetroRenderer
