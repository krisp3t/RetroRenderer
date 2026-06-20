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
#include <unordered_map>

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

    void StartSoftwareWorker();
    void StopSoftwareWorker();
    [[nodiscard]] std::shared_ptr<const RenderMeshSnapshot> GetOrCreateRenderMeshSnapshot(const Mesh& mesh);
    [[nodiscard]] std::shared_ptr<const Texture> GetOrCreateRenderTextureSnapshot(const Texture& texture);
    void ClearRenderResourceSnapshots();
    void SubmitSoftwareJob(const std::shared_ptr<const RenderPacket>& packet);
    void PresentCompletedSoftwareFrame();
    void RecordSoftwareFramePresented();
    void SoftwareWorkerLoop();
    void RenderSoftwareSync(const RenderPacket& packet);
    void StoreSoftwareFrame(const Buffer<Pixel>& buffer);
    void ClearSoftwareWorkerFrameState();

  private:
    std::shared_ptr<Config> p_Config_;
    std::shared_ptr<Stats> p_Stats_;
    const MaterialManager& m_MaterialManager;
    std::unique_ptr<SWRenderer> p_SWRenderer_ = nullptr;
    std::unordered_map<const Mesh*, std::shared_ptr<const RenderMeshSnapshot>> m_RenderMeshSnapshots;
    struct RenderTextureSnapshotCacheEntry {
        uint64_t revision = 0;
        std::shared_ptr<const Texture> texture;
    };
    std::unordered_map<const Texture*, RenderTextureSnapshotCacheEntry> m_RenderTextureSnapshots;
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
    std::deque<std::shared_ptr<CpuFrame>> m_CompletedSoftwareFrames;
    uint64_t m_NextSoftwareJobId = 0;
    bool m_SoftwareWorkerStopRequested = false;
    bool m_SoftwareWorkerBusy = false;
    static constexpr size_t kMaxBufferedSoftwareFrames = 3;
#endif
};

} // namespace RetroRenderer
