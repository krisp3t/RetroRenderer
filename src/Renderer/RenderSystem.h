#pragma once
#include "../Base/Color.h"
#include "../Base/Event.h"
#include "../Base/Stats.h"
#include "../Scene/Scene.h"
#include "IFramePresenter.h"
#include "IHardwareRenderer.h"
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

class RenderSystem {
  public:
    RenderSystem() = default;

    ~RenderSystem();

    bool Init();

    void BeforeFrame(const Color& clearColor);

    [[nodiscard]] std::vector<int>& BuildRenderQueue(Scene& scene, const Camera& camera);

    RenderOutput Render(const std::shared_ptr<Scene>& scene, const Camera& camera, const std::vector<int>& renderQueue);

    void Resize(const glm::ivec2& resolution);

    void Destroy();

    void OnLoadScene(const SceneLoadEvent& e);
    void OnResetScene();
    void OnSceneMutated();
    void OnTextureMutated();

    [[nodiscard]] ShaderHandle CompileShaders(const std::string& vertexCode, const std::string& fragmentCode);

  private:
    struct SoftwareRenderJob {
        std::shared_ptr<Scene> scene = nullptr;
        std::shared_ptr<Camera> camera = nullptr;
        std::vector<LightSnapshot> lights;
        std::vector<int> renderQueue;
        Config configSnapshot{};
        Color clearColor{};
        SoftwareMaterialState materialState{};
        std::optional<Texture> fallbackTexture;
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
    void SubmitSoftwareJob(const std::shared_ptr<Scene>& scene, const Camera& camera, const std::vector<int>& renderQueue);
    void PresentCompletedSoftwareFrame();
    void SoftwareWorkerLoop();
    RenderOutput RenderSoftwareSync(const std::shared_ptr<Scene>& scene, const Camera& camera, const std::vector<int>& renderQueue);
    [[nodiscard]] RenderOutput MakeSoftwareRenderOutput() const;
    void StoreSoftwareFrame(const Buffer<Pixel>& buffer);
    void ClearSoftwareWorkerFrameState();
    void SyncSoftwareWorkerForRenderDataMutation();

  private:
    std::unique_ptr<Scene> p_scene_ = nullptr;
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
