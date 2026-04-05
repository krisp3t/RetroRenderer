#pragma once

#include "../../Base/Color.h"
#include "../../Base/Config.h"
#include "../../Scene/Camera.h"
#include "../../Scene/Scene.h"
#include "../Buffer.h"
#include "../IRenderer.h"
#include "SoftwareLighting.h"
#include "Rasterizer.h"
#include <array>
#include <memory>
#include <vector>

namespace RetroRenderer {
class SWRenderer : public IRenderer {
  public:
    SWRenderer() = default;
    ~SWRenderer() = default;
    bool Init(int w, int h);
    bool Resize(int w, int h);
    void SetActiveCamera(const Camera& camera) override;
    void SetSceneLights(const std::vector<LightSnapshot>& lights) override;
    void SetFrameConfig(const Config& config);
    void SetMaterialState(const SoftwareMaterialState& materialState);
    void SetFallbackTexture(const Texture* texture);
    void DrawTriangularMesh(const Model* model) override;
    void DrawSkybox() override;
    void BeforeFrame(const Color& clearColor) override;
    GLuint EndFrame() override;
    GLuint CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) override {
        // TODO: implement
        return 0;
    }

    [[nodiscard]] const Buffer<Pixel>& GetFrameBuffer() const;

  private:
    struct DeferredTriangle {
        std::array<RasterVertex, 3> vertices{};
        const Texture* texture = nullptr;
        float sortKey = 0.0f;
    };

    std::unique_ptr<Buffer<Pixel>> m_FrameBuffer = nullptr;
    std::unique_ptr<Buffer<float>> m_DepthBuffer = nullptr;
    Camera* p_Camera = nullptr;
    std::vector<LightSnapshot> m_FrameLights;
    Config m_FrameConfigSnapshot{};
    SoftwareMaterialState m_FrameMaterialState{};
    const Texture* p_FrameFallbackTexture = nullptr;
    std::vector<DeferredTriangle> m_DeferredPs1Triangles;
    std::unique_ptr<Rasterizer> m_Rasterizer = nullptr;
    bool m_HasSkybox = false;
    int m_SkyboxFaceSize = 0;
    std::array<std::vector<Pixel>, 6> m_SkyboxFaces{};
    std::vector<Pixel> m_SkyboxCachePixels;
    CameraType m_SkyboxCacheCameraType = CameraType::PERSPECTIVE;
    glm::vec3 m_SkyboxCacheDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    float m_SkyboxCacheFov = 90.0f;
    float m_SkyboxCacheOrthoSize = 10.0f;
    size_t m_SkyboxCacheWidth = 0;
    size_t m_SkyboxCacheHeight = 0;
    bool m_SkyboxCacheValid = false;
};

} // namespace RetroRenderer
