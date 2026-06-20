#pragma once
#include "../Base/Color.h"
#include "../Base/Event.h"
#include "../Base/Stats.h"
#include "../Scene/Scene.h"
#include "CpuFrame.h"
#include "RenderServices.h"
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

class RenderSystem : public IRenderInvalidationSink {
  public:
    RenderSystem(std::shared_ptr<Config> config, std::shared_ptr<Stats> stats, const MaterialManager& materialManager);

    ~RenderSystem();

    bool Init();

    void BeforeFrame(const Color& clearColor);
    [[nodiscard]] bool PollSoftwareFrame();

    [[nodiscard]] std::shared_ptr<const RenderPacket> BuildRenderPacket(const std::shared_ptr<Scene>& scene,
                                                                        const Camera* camera);

    [[nodiscard]] std::shared_ptr<const CpuFrame> PrepareFrame(
        const std::shared_ptr<const RenderPacket>& packet);

    void Resize(const glm::ivec2& resolution);

    void Destroy();

    void OnLoadScene(const SceneLoadEvent& e);
    void OnResetScene();
    void OnSceneMutated() override;
    void OnTextureMutated() override;

  private:
    struct SoftwareRenderJob {
        std::shared_ptr<const RenderPacket> packet;
        uint64_t jobId = 0;
    };

    [[nodiscard]] bool EnsureSoftwareRenderer();
    void ReleaseSoftwareRenderer();
    void StartSoftwareWorker();
    void StopSoftwareWorker();
    void SubmitSoftwareJob(const std::shared_ptr<const RenderPacket>& packet);
    void PresentCompletedSoftwareFrame();
    void RecordSoftwareFramePresented();
    void SoftwareWorkerLoop();
    void RenderSoftwareSync(const RenderPacket& packet);
    void StoreSoftwareFrame(const Buffer<Pixel>& buffer, uint64_t frameId, uint64_t dataRevision);
    void ClearSoftwareWorkerFrameState();
    void UpdateSceneMemoryStats(const Scene* scene);
    void UpdateSoftwareMemoryStats();
    void ClearSoftwareMemoryStats();

  private:
    std::shared_ptr<Config> p_Config_;
    std::shared_ptr<Stats> p_Stats_;
    const MaterialManager& m_MaterialManager;
    std::unique_ptr<SWRenderer> p_SWRenderer_ = nullptr;
    SoftwareRendererMemoryStats m_SoftwareRendererMemoryStats{};
    std::shared_ptr<const CpuFrame> m_PresentedSoftwareFrame;
    Color m_SoftwareClearColor = Color::DefaultBackground();
    bool m_IsDestroyed = false;
    uint64_t m_FrameDataRevision = 1;
    uint64_t m_SceneResourceRevision = 1;
    uint64_t m_TextureResourceRevision = 1;

#if !defined(__EMSCRIPTEN__)
    std::thread m_SoftwareWorkerThread;
    std::condition_variable m_SoftwareWorkerCv;
    std::mutex m_SoftwareWorkerMutex;
    std::optional<SoftwareRenderJob> m_PendingSoftwareJob;
    std::shared_ptr<CpuFrame> m_CompletedSoftwareFrame;
    uint64_t m_NextSoftwareJobId = 0;
    bool m_SoftwareWorkerStopRequested = false;
    bool m_SoftwareWorkerBusy = false;
#endif
};

} // namespace RetroRenderer
