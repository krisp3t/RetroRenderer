#include "RenderSystem.h"
#include "../Scene/MaterialManager.h"
#include <KrisLogger/Logger.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <unordered_map>
#include <utility>

namespace RetroRenderer
{
    namespace
    {
        using TimingClock = std::chrono::steady_clock;

        uint64_t ElapsedNanoseconds(TimingClock::time_point start)
        {
            return static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(TimingClock::now() - start).count());
        }

        uint64_t ClockNanoseconds()
        {
            return static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(TimingClock::now().time_since_epoch()).count());
        }

        const MaterialParameterOverride* FindParameterOverride(const SceneMaterial& material, const std::string& name)
        {
            for (const MaterialParameterOverride& overrideValue : material.parameterOverrides)
            {
                if (overrideValue.name == name)
                {
                    return &overrideValue;
                }
            }
            return nullptr;
        }

        std::shared_ptr<const Texture> FindTextureBinding(const SceneMaterial& material, const std::string& slotName)
        {
            for (const MaterialTextureBinding& binding : material.textureBindings)
            {
                if (binding.slotName == slotName && binding.texture && binding.texture->HasCpuPixels())
                {
                    return binding.texture;
                }
            }
            return nullptr;
        }

        void ApplyPipelineOverrides(MaterialPipelineState& state, const SceneMaterialOverrides& overrides)
        {
            if (overrides.blendMode.has_value())
            {
                state.blendMode = *overrides.blendMode;
            }
            if (overrides.cullMode.has_value())
            {
                state.cullMode = *overrides.cullMode;
            }
            if (overrides.depthTest.has_value())
            {
                state.depthTest = *overrides.depthTest;
            }
            if (overrides.depthWrite.has_value())
            {
                state.depthWrite = *overrides.depthWrite;
            }
            if (overrides.alphaCutoff.has_value())
            {
                state.alphaCutoff = *overrides.alphaCutoff;
            }
        }
    } // namespace

    RenderSystem::RenderSystem(std::shared_ptr<Config> config,
                               std::shared_ptr<Stats> stats,
                               const MaterialManager& materialManager)
        : p_Config_(std::move(config)), p_Stats_(std::move(stats)), m_MaterialManager(materialManager)
    {
    }

    RenderSystem::~RenderSystem()
    {
        Destroy();
    }

    bool RenderSystem::Init()
    {
        m_IsDestroyed = false;
        assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
        ClearSoftwareWorkerFrameState();
        ClearSoftwareMemoryStats();
        UpdateSceneMemoryStats(nullptr);
        return true;
    }

    void RenderSystem::BeforeFrame(const Color& clearColor)
    {
        assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
        m_SoftwareClearColor = clearColor;
    }

    bool RenderSystem::PollSoftwareFrame()
    {
#if !defined(__EMSCRIPTEN__)
        PresentCompletedSoftwareFrame();
#endif
        return m_PresentedSoftwareFrame != nullptr;
    }

    std::shared_ptr<const RenderPacket> RenderSystem::BuildRenderPacket(const std::shared_ptr<Scene>& scene,
                                                                        const Camera* camera)
    {
        assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
        auto mutablePacket = std::make_shared<RenderPacket>();
        RenderPacket& packet = *mutablePacket;
        packet.configSnapshot = *p_Config_;
        packet.clearColor = p_Config_->renderer.clearColor;
        packet.dataRevision = m_FrameDataRevision;
        packet.sceneResourceRevision = m_SceneResourceRevision;
        packet.textureResourceRevision = m_TextureResourceRevision;

        UpdateSceneMemoryStats(scene.get());
        if (!scene)
        {
            return mutablePacket;
        }

        packet.hasScene = true;
        if (camera != nullptr)
        {
            packet.camera = *camera;
        }
        scene->BuildLightSnapshots(packet.lights);

        const auto captureTextureId = [&](const std::shared_ptr<const Texture>& texture) -> FrameTextureId
        {
            if (!texture || !texture->HasCpuPixels())
            {
                return kInvalidFrameTextureId;
            }

            const FrameTextureId textureId = static_cast<FrameTextureId>(packet.textures.size());
            packet.textures.push_back(texture);
            return textureId;
        };

        const auto& visibleModels = scene->GetVisibleModels();
        size_t renderItemCount = 0;
        for (int modelIx : visibleModels)
        {
            if (modelIx < 0 || static_cast<size_t>(modelIx) >= scene->GetModelCount())
            {
                continue;
            }
            renderItemCount += scene->GetModel(static_cast<size_t>(modelIx)).GetMeshCount();
        }
        packet.textures.reserve(std::max<size_t>(packet.textures.capacity(), std::min(renderItemCount, size_t{8})));
        packet.items.reserve(renderItemCount);

        std::unordered_map<const Texture*, FrameTextureId> textureIds;
        const auto getTextureId = [&](const std::shared_ptr<const Texture>& texture) -> FrameTextureId
        {
            if (!texture || !texture->HasCpuPixels())
            {
                return kInvalidFrameTextureId;
            }
            auto it = textureIds.find(texture.get());
            if (it != textureIds.end())
            {
                return it->second;
            }
            const FrameTextureId textureId = captureTextureId(texture);
            if (textureId != kInvalidFrameTextureId)
            {
                textureIds.emplace(texture.get(), textureId);
            }
            return textureId;
        };

        std::unordered_map<SceneMaterialHandle, FrameMaterialId> materialIds;

        for (int modelIx : visibleModels)
        {
            if (modelIx < 0 || static_cast<size_t>(modelIx) >= scene->GetModelCount())
            {
                continue;
            }

            const Model& model = scene->GetModel(static_cast<size_t>(modelIx));
            for (size_t meshIx = 0; meshIx < model.GetMeshCount(); meshIx++)
            {
                const Mesh& mesh = model.GetMesh(meshIx);
                const std::shared_ptr<const MeshGeometryData>& geometry = mesh.GetGeometry();
                if (!geometry || geometry->vertices.empty() || geometry->indices.empty())
                {
                    continue;
                }

                RenderItem item{};
                item.geometry = geometry;
                item.worldTransform = model.GetWorldTransform();

                const SceneMaterialHandle sceneMaterialHandle = mesh.GetMaterialHandle();
                auto materialIt = materialIds.find(sceneMaterialHandle);
                if (materialIt == materialIds.end())
                {
                    const SceneMaterial* sceneMaterial = scene->GetMaterial(sceneMaterialHandle);
                    if (sceneMaterial == nullptr)
                    {
                        continue;
                    }

                    FrameMaterialState frameMaterial{};
                    frameMaterial.compiledTemplate = m_MaterialManager.GetCompiledTemplate(*sceneMaterial);
                    if (!frameMaterial.compiledTemplate)
                    {
                        continue;
                    }
                    frameMaterial.pipelineState = frameMaterial.compiledTemplate->pipelineState;
                    ApplyPipelineOverrides(frameMaterial.pipelineState, sceneMaterial->pipelineOverrides);
                    frameMaterial.parameterValues.reserve(frameMaterial.compiledTemplate->parameters.size());
                    for (const MaterialParameterDesc& parameter : frameMaterial.compiledTemplate->parameters)
                    {
                        const MaterialParameterOverride* overrideValue = FindParameterOverride(
                            *sceneMaterial, parameter.name);
                        frameMaterial.parameterValues.push_back(
                            overrideValue != nullptr ? overrideValue->value.data : parameter.defaultValue.data);
                    }
                    frameMaterial.textureIds.reserve(frameMaterial.compiledTemplate->samplers.size());
                    for (const MaterialSamplerDesc& sampler : frameMaterial.compiledTemplate->samplers)
                    {
                        frameMaterial.textureIds.push_back(
                            getTextureId(FindTextureBinding(*sceneMaterial, sampler.name)));
                    }

                    const FrameMaterialId materialId = static_cast<FrameMaterialId>(packet.materials.size());
                    packet.materials.push_back(std::move(frameMaterial));
                    materialIt = materialIds.emplace(sceneMaterialHandle, materialId).first;
                }

                item.materialId = materialIt->second;
                packet.items.push_back(std::move(item));
            }
        }

        return mutablePacket;
    }

    std::shared_ptr<const CpuFrame> RenderSystem::PrepareFrame(const std::shared_ptr<const RenderPacket>& packet)
    {
        assert(packet && packet->hasScene && "Render called with empty render packet");
        assert(p_Stats_ != nullptr && "RenderSystem requires stats");
        const auto renderSystemStart = TimingClock::now();
        p_Stats_->Reset();

        if (packet->configSnapshot.renderer.selectedRenderer == Config::RendererType::SOFTWARE)
        {
            p_Stats_->lastGlRenderNs.store(0, std::memory_order_relaxed);
            if (!EnsureSoftwareRenderer())
            {
                p_Stats_->lastRenderSystemNs.store(ElapsedNanoseconds(renderSystemStart), std::memory_order_relaxed);
                return nullptr;
            }
#if defined(__EMSCRIPTEN__)
            RenderSoftwareSync(*packet);
            p_Stats_->lastRenderSystemNs.store(ElapsedNanoseconds(renderSystemStart), std::memory_order_relaxed);
            return m_PresentedSoftwareFrame;
#else
            SubmitSoftwareJob(packet);
            PresentCompletedSoftwareFrame();
            p_Stats_->lastRenderSystemNs.store(ElapsedNanoseconds(renderSystemStart), std::memory_order_relaxed);
            return m_PresentedSoftwareFrame;
#endif
        }

        ReleaseSoftwareRenderer();
        p_Stats_->lastSoftwarePacketCopyNs.store(0, std::memory_order_relaxed);
        p_Stats_->lastSoftwareWorkerRenderNs.store(0, std::memory_order_relaxed);
        p_Stats_->lastSoftwareWorkerCopyNs.store(0, std::memory_order_relaxed);
        p_Stats_->lastSoftwareFramePresentIntervalNs.store(0, std::memory_order_relaxed);
        p_Stats_->lastRenderSystemNs.store(ElapsedNanoseconds(renderSystemStart), std::memory_order_relaxed);
        return nullptr;
    }

    void RenderSystem::Resize(const glm::ivec2& resolution)
    {
        assert(resolution.x > 0 && resolution.y > 0 && "Tried to resize renderer with invalid resolution");
        assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
        p_Config_->renderer.resolution = resolution;

        LOGI("Resizing output image to %d x %d", resolution.x, resolution.y);
        if (!p_SWRenderer_)
        {
            return;
        }

#if !defined(__EMSCRIPTEN__)
        StopSoftwareWorker();
#endif
        p_SWRenderer_->Resize(resolution.x, resolution.y);
        m_SoftwareRendererMemoryStats = p_SWRenderer_->EstimateResidentMemory();
        ClearSoftwareWorkerFrameState();
#if !defined(__EMSCRIPTEN__)
        StartSoftwareWorker();
#endif
        UpdateSoftwareMemoryStats();
    }

    void RenderSystem::Destroy()
    {
        if (m_IsDestroyed)
        {
            return;
        }
        m_IsDestroyed = true;
        ReleaseSoftwareRenderer();
    }

    void RenderSystem::OnLoadScene(const SceneLoadEvent& e)
    {
        (void)e;
        UpdateSceneMemoryStats(nullptr);
        ++m_FrameDataRevision;
        ++m_SceneResourceRevision;
        ++m_TextureResourceRevision;
    }

    void RenderSystem::OnResetScene()
    {
        UpdateSceneMemoryStats(nullptr);
        ++m_FrameDataRevision;
        ++m_SceneResourceRevision;
        ++m_TextureResourceRevision;
    }

    void RenderSystem::OnSceneMutated()
    {
        ++m_FrameDataRevision;
    }

    void RenderSystem::OnTextureMutated()
    {
        ++m_FrameDataRevision;
        ++m_TextureResourceRevision;
    }

    bool RenderSystem::EnsureSoftwareRenderer()
    {
        if (p_SWRenderer_)
        {
            return true;
        }

        assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
        auto renderer = std::make_unique<SWRenderer>();
        const glm::ivec2 resolution = p_Config_->renderer.resolution;
        assert(resolution.x > 0 && resolution.y > 0 && "Tried to initialize software renderer with invalid resolution");
        if (!renderer->Init(resolution.x, resolution.y))
        {
            LOGE("Failed to initialize SWRenderer");
            return false;
        }

        m_SoftwareRendererMemoryStats = renderer->EstimateResidentMemory();
        p_SWRenderer_ = std::move(renderer);
#if !defined(__EMSCRIPTEN__)
        StartSoftwareWorker();
#endif
        UpdateSoftwareMemoryStats();
        return true;
    }

    void RenderSystem::ReleaseSoftwareRenderer()
    {
        StopSoftwareWorker();
        ClearSoftwareWorkerFrameState();
        if (p_SWRenderer_)
        {
            p_SWRenderer_->Destroy();
            p_SWRenderer_.reset();
        }
        ClearSoftwareMemoryStats();
    }

    void RenderSystem::StartSoftwareWorker()
    {
#if !defined(__EMSCRIPTEN__)
        if (!p_SWRenderer_ || m_SoftwareWorkerThread.joinable())
        {
            return;
        }
        m_SoftwareWorkerStopRequested = false;
        m_SoftwareWorkerThread = std::thread(&RenderSystem::SoftwareWorkerLoop, this);
#endif
    }

    void RenderSystem::StopSoftwareWorker()
    {
#if !defined(__EMSCRIPTEN__)
        if (!m_SoftwareWorkerThread.joinable())
        {
            return;
        }

        bool cancelledPending = false;
        {
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            m_SoftwareWorkerStopRequested = true;
            cancelledPending = m_PendingSoftwareJob.has_value();
            m_PendingSoftwareJob.reset();
        }
        if (cancelledPending)
        {
            p_Stats_->swJobsCancelled.fetch_add(1, std::memory_order_relaxed);
        }

        m_SoftwareWorkerCv.notify_one();
        m_SoftwareWorkerThread.join();

        {
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            m_SoftwareWorkerStopRequested = false;
            m_PendingSoftwareJob.reset();
            m_CompletedSoftwareFrame.reset();
            m_SoftwareWorkerBusy = false;
        }
#endif
    }

    void RenderSystem::SubmitSoftwareJob(const std::shared_ptr<const RenderPacket>& packet)
    {
#if !defined(__EMSCRIPTEN__)
        if (!packet || !packet->hasScene || !p_SWRenderer_)
        {
            return;
        }
        assert(p_Stats_ != nullptr && "RenderSystem requires stats");

        SoftwareRenderJob job{};
        const auto softwarePacketCopyStart = TimingClock::now();
        job.packet = packet;
        p_Stats_->lastSoftwarePacketCopyNs.store(ElapsedNanoseconds(softwarePacketCopyStart),
                                                 std::memory_order_relaxed);
        if (!job.packet || !job.packet->hasScene)
        {
            return;
        }

        bool replacedPending = false;
        {
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            job.jobId = ++m_NextSoftwareJobId;
            replacedPending = m_PendingSoftwareJob.has_value();
            m_PendingSoftwareJob = std::move(job);
        }

        if (replacedPending)
        {
            p_Stats_->swJobsReplacedPending.fetch_add(1, std::memory_order_relaxed);
        }
        p_Stats_->swJobsSubmitted.fetch_add(1, std::memory_order_relaxed);
        m_SoftwareWorkerCv.notify_one();
#endif
    }

    void RenderSystem::PresentCompletedSoftwareFrame()
    {
#if !defined(__EMSCRIPTEN__)
        assert(p_Stats_ != nullptr && "RenderSystem requires stats");

        std::shared_ptr<CpuFrame> completedFrame;
        {
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            if (!m_CompletedSoftwareFrame)
            {
                return;
            }
            if (m_CompletedSoftwareFrame->dataRevision < m_FrameDataRevision)
            {
                m_CompletedSoftwareFrame.reset();
                UpdateSoftwareMemoryStats();
                return;
            }
            completedFrame = std::move(m_CompletedSoftwareFrame);
        }
        if (!completedFrame || completedFrame->pixels.empty() || completedFrame->width == 0 || completedFrame->height ==
            0)
        {
            UpdateSoftwareMemoryStats();
            return;
        }

        if (completedFrame->pitch == 0)
        {
            completedFrame->pitch = completedFrame->width * sizeof(Pixel);
        }
        m_PresentedSoftwareFrame = std::move(completedFrame);
        UpdateSoftwareMemoryStats();
        RecordSoftwareFramePresented();
#endif
    }

    void RenderSystem::RecordSoftwareFramePresented()
    {
        assert(p_Stats_ != nullptr && "RenderSystem requires stats");
        const uint64_t nowNs = ClockNanoseconds();
        const uint64_t previousNs = p_Stats_->lastSoftwareFramePresentedNs.exchange(nowNs, std::memory_order_relaxed);
        if (previousNs != 0 && nowNs > previousNs)
        {
            p_Stats_->lastSoftwareFramePresentIntervalNs.store(nowNs - previousNs, std::memory_order_relaxed);
        }
        p_Stats_->swFramesPresented.fetch_add(1, std::memory_order_relaxed);
    }

    void RenderSystem::SoftwareWorkerLoop()
    {
#if !defined(__EMSCRIPTEN__)
        assert(p_Stats_ != nullptr && "RenderSystem requires stats");

        while (true)
        {
            SoftwareRenderJob job{};
            {
                std::unique_lock<std::mutex> lock(m_SoftwareWorkerMutex);
                m_SoftwareWorkerCv.wait(lock, [this]()
                {
                    return m_SoftwareWorkerStopRequested || m_PendingSoftwareJob.has_value();
                });
                if (m_SoftwareWorkerStopRequested && !m_PendingSoftwareJob.has_value())
                {
                    return;
                }
                if (!m_PendingSoftwareJob.has_value())
                {
                    continue;
                }
                job = std::move(*m_PendingSoftwareJob);
                m_PendingSoftwareJob.reset();
                m_SoftwareWorkerBusy = true;
            }

            const auto workerRenderStart = TimingClock::now();
            p_SWRenderer_->RenderFrame(*job.packet);
            p_Stats_->lastSoftwareWorkerRenderNs.
                      store(ElapsedNanoseconds(workerRenderStart), std::memory_order_relaxed);

            const auto workerCopyStart = TimingClock::now();
            const auto& buffer = p_SWRenderer_->GetFrameBuffer();
            auto finishedFrame = std::make_shared<CpuFrame>();
            finishedFrame->width = buffer.width;
            finishedFrame->height = buffer.height;
            finishedFrame->pitch = buffer.pitch;
            finishedFrame->frameId = job.jobId;
            finishedFrame->dataRevision = job.packet->dataRevision;
            finishedFrame->pixels.resize(buffer.GetCount());
            if (!finishedFrame->pixels.empty())
            {
                std::memcpy(finishedFrame->pixels.data(), buffer.data, finishedFrame->pixels.size() * sizeof(Pixel));
            }
            p_Stats_->lastSoftwareWorkerCopyNs.store(ElapsedNanoseconds(workerCopyStart), std::memory_order_relaxed);
            const SoftwareRendererMemoryStats rendererMemoryStats = p_SWRenderer_->EstimateResidentMemory();

            bool stopRequested = false;
            bool replacedReady = false;
            {
                std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
                stopRequested = m_SoftwareWorkerStopRequested;
                m_SoftwareWorkerBusy = false;
                m_SoftwareRendererMemoryStats = rendererMemoryStats;
                if (!stopRequested)
                {
                    replacedReady = static_cast<bool>(m_CompletedSoftwareFrame);
                    m_CompletedSoftwareFrame = std::move(finishedFrame);
                }
            }

            if (stopRequested)
            {
                p_Stats_->swJobsCancelled.fetch_add(1, std::memory_order_relaxed);
                UpdateSoftwareMemoryStats();
                return;
            }
            if (replacedReady)
            {
                p_Stats_->swFramesReplacedReady.fetch_add(1, std::memory_order_relaxed);
            }
            p_Stats_->swJobsCompleted.fetch_add(1, std::memory_order_relaxed);
            UpdateSoftwareMemoryStats();
        }
#endif
    }

    void RenderSystem::RenderSoftwareSync(const RenderPacket& packet)
    {
        if (!p_SWRenderer_)
        {
            return;
        }

        p_Stats_->lastSoftwarePacketCopyNs.store(0, std::memory_order_relaxed);

        const auto workerRenderStart = TimingClock::now();
        p_SWRenderer_->RenderFrame(packet);
        p_Stats_->lastSoftwareWorkerRenderNs.store(ElapsedNanoseconds(workerRenderStart), std::memory_order_relaxed);
        m_SoftwareRendererMemoryStats = p_SWRenderer_->EstimateResidentMemory();

        const auto workerCopyStart = TimingClock::now();
        const auto& buffer = p_SWRenderer_->GetFrameBuffer();
        StoreSoftwareFrame(buffer, 0, packet.dataRevision);
        p_Stats_->lastSoftwareWorkerCopyNs.store(ElapsedNanoseconds(workerCopyStart), std::memory_order_relaxed);
        RecordSoftwareFramePresented();
    }

    void RenderSystem::StoreSoftwareFrame(const Buffer<Pixel>& buffer, uint64_t frameId, uint64_t dataRevision)
    {
        auto frame = std::make_shared<CpuFrame>();
        frame->width = buffer.width;
        frame->height = buffer.height;
        frame->pitch = buffer.pitch;
        frame->frameId = frameId;
        frame->dataRevision = dataRevision;
        frame->pixels.resize(buffer.GetCount());
        if (!frame->pixels.empty())
        {
            std::memcpy(frame->pixels.data(), buffer.data, frame->pixels.size() * sizeof(Pixel));
        }
        m_PresentedSoftwareFrame = std::move(frame);
        UpdateSoftwareMemoryStats();
    }

    void RenderSystem::ClearSoftwareWorkerFrameState()
    {
#if !defined(__EMSCRIPTEN__)
        {
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            m_PendingSoftwareJob.reset();
            m_CompletedSoftwareFrame.reset();
            m_SoftwareWorkerBusy = false;
        }
#endif
        m_PresentedSoftwareFrame.reset();
        if (p_Stats_)
        {
            p_Stats_->lastSoftwareFramePresentedNs.store(0, std::memory_order_relaxed);
            p_Stats_->lastSoftwareFramePresentIntervalNs.store(0, std::memory_order_relaxed);
        }
        UpdateSoftwareMemoryStats();
    }

    void RenderSystem::UpdateSceneMemoryStats(const Scene* scene)
    {
        if (!p_Stats_)
        {
            return;
        }

        if (scene == nullptr)
        {
            p_Stats_->sceneGeometryCpuBytes = 0;
            p_Stats_->sceneTextureCpuBytes = 0;
            return;
        }

        const SceneMemoryStats stats = scene->EstimateResidentMemory();
        p_Stats_->sceneGeometryCpuBytes = stats.geometryBytes;
        p_Stats_->sceneTextureCpuBytes = stats.textureBytes;
    }

    void RenderSystem::UpdateSoftwareMemoryStats()
    {
        if (!p_Stats_)
        {
            return;
        }

        if (!p_SWRenderer_)
        {
            ClearSoftwareMemoryStats();
            return;
        }

        SoftwareRendererMemoryStats rendererStats = m_SoftwareRendererMemoryStats;
        uint64_t readyFrameBytes = 0;
#if !defined(__EMSCRIPTEN__)
        {
            /*
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            rendererStats = m_SoftwareRendererMemoryStats;
            if (m_CompletedSoftwareFrame) {
                readyFrameBytes = m_CompletedSoftwareFrame->EstimateResidentMemory();
            }
            */
        }
#endif

        p_Stats_->softwareFramebufferColorBytes = rendererStats.framebufferColorBytes;
        p_Stats_->softwareDepthBufferBytes = rendererStats.depthBufferBytes;
        p_Stats_->softwareScratchBytes = rendererStats.scratchBytes;
        p_Stats_->softwareDeferredTriangleBytes = rendererStats.deferredTriangleBytes;
        p_Stats_->softwareSkyboxBytes = rendererStats.skyboxFaceBytes + rendererStats.skyboxCacheBytes;
        p_Stats_->softwareReadyFrameBytes = readyFrameBytes;
        p_Stats_->softwarePresentedFrameBytes = m_PresentedSoftwareFrame
                                                    ? m_PresentedSoftwareFrame->EstimateResidentMemory()
                                                    : 0;
    }

    void RenderSystem::ClearSoftwareMemoryStats()
    {
        if (!p_Stats_)
        {
            return;
        }

        p_Stats_->softwareFramebufferColorBytes = 0;
        p_Stats_->softwareDepthBufferBytes = 0;
        p_Stats_->softwareScratchBytes = 0;
        p_Stats_->softwareDeferredTriangleBytes = 0;
        p_Stats_->softwareSkyboxBytes = 0;
        p_Stats_->softwareReadyFrameBytes = 0;
        p_Stats_->softwarePresentedFrameBytes = 0;
        m_SoftwareRendererMemoryStats = {};
    }
} // namespace RetroRenderer
