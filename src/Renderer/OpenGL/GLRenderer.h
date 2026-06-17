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
class GLRenderer : public IHardwareRenderer {
  public:
    GLRenderer() = default;

    ~GLRenderer() = default;

    bool Init(TextureHandle fbTex, int w, int h) override;

    void Resize(TextureHandle newFbTex, int w, int h) override;

    void Destroy() override;
    void InvalidateSceneResources() override;
    void InvalidateTextureResources() override;
    void RenderFrame(const FrameSnapshot& frame) override;

    void SetActiveCamera(const Camera& camera) override;
    void SetSceneLights(const std::vector<LightSnapshot>& lights) override;

    void DrawTriangularMesh(const Model* model) override;

    void DrawSkybox() override;
    void DrawGridGizmo() override;

    void BeforeFrame(const Color& clearColor) override;

    void EndFrame() override;

    GLuint CreateShaderProgram();
    ShaderHandle CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) override;

  private:
    static void DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar* message, const void* userParam);
    bool CreateFramebuffer(TextureHandle fbTex, int w, int h);
    GLuint CompileShader(GLenum shaderType, const char* shaderSource);
    void CheckShaderErrors(GLuint shader, const std::string& type);
    void CreateFallbackTexture();
    GLuint CreateCubemap(const std::string& path);
    GLuint CreateSkyboxVAO();
    void DestroyFramebufferResources();
    void DestroyRendererResources();
    void DrawMesh(const Mesh& mesh, const glm::mat4& worldTransform, const Texture* texture, const FrameMaterialState& materialState,
                  const Config& configSnapshot);

  private:
    Camera m_FrameCameraSnapshot{};
    Camera* p_Camera = nullptr;
    std::vector<LightSnapshot> m_SceneLights;

    GLuint p_FrameBufferTexture = 0;
    GLuint m_FrameBuffer = 0;
    GLuint m_DepthBuffer = 0;
    GLuint m_FallbackTexture = 0;
    GLuint m_SkyboxTexture = 0;
    GLuint m_SkyboxVAO = 0;
    GLuint m_SkyboxVBO = 0;
    ShaderProgram m_SkyboxProgram;
    ShaderHandle m_GridProgram;
    GLuint m_GridVAO = 0;
    GLuint m_GridVBO = 0;
    GLsizei m_GridVertexCount = 0;
    Config m_FrameConfigSnapshot{};
    int m_FrameWidth = 0;
    int m_FrameHeight = 0;
    GLMeshResourceCache m_MeshResources;
    GLTextureResourceCache m_TextureResources;
};
} // namespace RetroRenderer
