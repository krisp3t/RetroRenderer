#pragma once

#include "../../Base/Color.h"
#include "../../Base/Config.h"
#include "../../Scene/Camera.h"
#include "../../Scene/Scene.h"
#include "../Buffer.h"
#include "../IRenderer.h"
#include "Rasterizer.h"
#include <memory>

namespace RetroRenderer {
class SWRenderer : public IRenderer {
  public:
    SWRenderer() = default;
    ~SWRenderer() = default;
    bool Init(int w, int h);
    bool Resize(int w, int h);
    void SetActiveCamera(const Camera& camera) override;
    void SetFrameConfig(const Config& config);
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
    std::unique_ptr<Buffer<Pixel>> m_FrameBuffer = nullptr;
    std::unique_ptr<Buffer<float>> m_DepthBuffer = nullptr;
    Camera* p_Camera = nullptr;
    Config m_FrameConfigSnapshot{};
    std::unique_ptr<Rasterizer> m_Rasterizer = nullptr;
};

} // namespace RetroRenderer
