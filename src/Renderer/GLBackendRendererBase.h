#pragma once

#include "../Scene/MaterialManager.h"
#include "GLBackendCommon.h"
#include "GLMeshResourceCache.h"
#include "GLTextureResourceCache.h"
#include "IHardwareRenderer.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace RetroRenderer {

class GLBackendRendererBase : public IHardwareRenderer {
  public:
    GLBackendRendererBase() = default;
    ~GLBackendRendererBase() override = default;

    void Resize(TextureHandle newFbTex, int w, int h) override;

    void Destroy() override;
    void InvalidateSceneResources() override;
    void InvalidateTextureResources() override;
    void RenderFrame(const RenderPacket& packet) override;

    void SetActiveCamera(const Camera& camera) override;
    void SetSceneLights(const std::vector<LightSnapshot>& lights) override;

    void DrawTriangularMesh(const Model* model) override;
    void DrawSkybox() override;

    void BeforeFrame(const Color& clearColor) override;
    void EndFrame() override;

    ShaderHandle CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) override;

  protected:
    bool InitializeBackend(TextureHandle fbTex, int w, int h, const std::string& cubemapPath);

    bool CreateFramebuffer(TextureHandle fbTex, int w, int h);
    void CreateFallbackTexture();
    GLuint CreateCubemap(const std::string& path);
    GLuint CreateSkyboxVAO();
    void DestroyFramebufferResources();
    void DestroyRendererResources();
    void DrawMesh(const Mesh& mesh,
                  const glm::mat4& worldTransform,
                  const Texture* texture,
                  const FrameMaterialState& materialState,
                  const Config& configSnapshot);
    void DrawMesh(const RenderMeshSnapshot& mesh,
                  const glm::mat4& worldTransform,
                  const Texture* texture,
                  const FrameMaterialState& materialState,
                  const Config& configSnapshot);
    void DrawMeshGpuResources(const GLMeshResourceCache::MeshGpuResources& gpuMesh,
                              const glm::mat4& worldTransform,
                              const Texture* texture,
                              const FrameMaterialState& materialState,
                              const Config& configSnapshot);

    virtual std::string_view GetShaderPrefix() const = 0;
    virtual const char* GetRendererLogLabel() const = 0;
    virtual void ConfigureBackendDebugState() {
    }
    virtual void InitializeBackendResources() {
    }
    virtual void DestroyBackendResources() {
    }
    virtual void ApplyBackendFrameState() {
    }
    virtual void ResetBackendFrameState() {
    }
    virtual void RenderBackendOverlays() {
    }

  protected:
    Camera m_FrameCameraSnapshot{};
    const Camera* m_ActiveCamera = nullptr;
    std::vector<LightSnapshot> m_SceneLights;
    FrameMaterialState m_FrameMaterialState{};

    GLuint m_FrameBufferTexture = 0;
    GLuint m_FrameBuffer = 0;
    GLuint m_DepthBuffer = 0;
    GLuint m_FallbackTexture = 0;
    GLuint m_SkyboxTexture = 0;
    GLuint m_SkyboxVAO = 0;
    GLuint m_SkyboxVBO = 0;
    ShaderProgram m_SkyboxProgram;
    Config m_FrameConfigSnapshot{};
    int m_FrameWidth = 0;
    int m_FrameHeight = 0;
    GLMeshResourceCache m_MeshResources;
    GLTextureResourceCache m_TextureResources;
    GLProgramUniformCache m_UniformCache;
    uint64_t m_SceneResourceRevision = 0;
    uint64_t m_TextureResourceRevision = 0;

  private:
    GLuint CompileShaderStage(GLenum shaderType, const std::string& shaderSource);
    GLuint ResolveShaderProgram(const std::shared_ptr<const RenderShaderSnapshot>& shader);
    void DestroyMaterialShaders();
    bool CheckShaderErrors(GLuint object, const char* stage, bool isProgram) const;

    std::unordered_map<std::shared_ptr<const RenderShaderSnapshot>, GLuint> m_MaterialShaders;
};

} // namespace RetroRenderer
