#pragma once
#include "../Base/Color.h"
#include "../Base/Event.h"
#include "../Base/Stats.h"
#include "../Scene/Scene.h"
#include "IFramePresenter.h"
#include "IHardwareRenderer.h"
#include "IRenderer.h"
#include "RenderServices.h"
#include "RenderOutput.h"
#include "ShaderHandle.h"
#include "Software/SWRenderer.h"
#if !defined(__EMSCRIPTEN__)
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#endif
#include <glm/vec2.hpp>
#include <cstdint>
#include <memory>

namespace RetroRenderer {
class MaterialManager;

class RenderSystem : public IRenderInvalidationSink, public IShaderCompiler {
  public:
    RenderSystem(std::shared_ptr<Config> config, std::shared_ptr<Stats> stats, const MaterialManager& materialManager);

    ~RenderSystem();

    bool Init();

    void BeforeFrame(const Color& clearColor);

    [[nodiscard]] FrameSnapshot BuildFrameSnapshot(const std::shared_ptr<Scene>& scene, const Camera& camera) const;

    RenderOutput Render(const FrameSnapshot& frame);

    void Resize(const glm::ivec2& resolution);

    void Destroy();

    void OnLoadScene(const SceneLoadEvent& e);
    void OnResetScene();
    void OnSceneMutated() override;
    void OnTextureMutated() override;

    [[nodiscard]] ShaderHandle CompileShaders(const std::string& vertexCode,
                                             const std::string& fragmentCode) override;

  private:
    struct SoftwareRenderJob {
        FrameSnapshot frame;
        std::optional<Texture> textureOverride;
        uint64_t jobId = 0;
    };

    struct SoftwareCompletedFrame {
        std::vector<Pixel> pixels;
        size_t width = 0;
        size_t height = 0;
        size_t pitch = 0;
        uint64_t jobId = 0;
    };

    void StartSoftwareWorker();
    void StopSoftwareWorker();
    void SubmitSoftwareJob(const FrameSnapshot& frame);
    void PresentCompletedSoftwareFrame();
    void SoftwareWorkerLoop();
    RenderOutput RenderSoftwareSync(const FrameSnapshot& frame);
    [[nodiscard]] RenderOutput MakeSoftwareRenderOutput() const;
    void StoreSoftwareFrame(const Buffer<Pixel>& buffer);
    void ClearSoftwareWorkerFrameState();
    void SyncSoftwareWorkerForRenderDataMutation();

  private:
    std::shared_ptr<Config> p_Config_;
    std::shared_ptr<Stats> p_Stats_;
    const MaterialManager& m_MaterialManager;
    std::unique_ptr<SWRenderer> p_SWRenderer_ = nullptr;
    std::unique_ptr<IHardwareRenderer> p_GLRenderer_ = nullptr;
    IRenderer* p_activeRenderer_ = nullptr;

    std::unique_ptr<IFramePresenter> m_GLFramePresenter;
    SoftwareCompletedFrame m_PresentedSoftwareFrame;
    Color m_SoftwareClearColor = Color::DefaultBackground();
    bool m_IsDestroyed = false;

#if !defined(__EMSCRIPTEN__)
    std::thread m_SoftwareWorkerThread;
    std::condition_variable m_SoftwareWorkerCv;
    std::mutex m_SoftwareWorkerMutex;
    std::optional<SoftwareRenderJob> m_PendingSoftwareJob;
    std::deque<SoftwareCompletedFrame> m_CompletedSoftwareFrames;
    uint64_t m_NextSoftwareJobId = 0;
    bool m_SoftwareWorkerStopRequested = false;
    static constexpr size_t kMaxBufferedSoftwareFrames = 3;
#endif
};

} // namespace RetroRenderer
