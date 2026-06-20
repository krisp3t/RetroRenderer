#include "RenderSystem.h"
#include "../Scene/MaterialManager.h"
#include <KrisLogger/Logger.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <unordered_map>
#include <utility>

namespace RetroRenderer {
namespace {
using TimingClock = std::chrono::steady_clock;

uint64_t ElapsedNanoseconds(TimingClock::time_point start) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(TimingClock::now() - start).count());
}

uint64_t ClockNanoseconds() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(TimingClock::now().time_since_epoch()).count());
}

FrameMaterialState CaptureFrameMaterialState(const MaterialManager::Material& currentMaterial) {
    FrameMaterialState state{};
    state.shader = currentMaterial.shaderProgram.source;
    state.lightColor = currentMaterial.lightColor;
    state.useVertexColor = currentMaterial.name == "phong-vc" ||
                           currentMaterial.shaderProgram.vertexPath.find("phong-vc") != std::string::npos ||
                           currentMaterial.shaderProgram.fragmentPath.find("phong-vc") != std::string::npos;
    if (currentMaterial.phongParams.has_value()) {
        state.enablePhong = true;
        state.ambientStrength = currentMaterial.phongParams->ambientStrength;
        state.specularStrength = currentMaterial.phongParams->specularStrength;
        state.shininess = currentMaterial.phongParams->shininess;
    }
    return state;
}
} // namespace

RenderSystem::RenderSystem(std::shared_ptr<Config> config,
                           std::shared_ptr<Stats> stats,
                           const MaterialManager& materialManager)
    : p_Config_(std::move(config)), p_Stats_(std::move(stats)), m_MaterialManager(materialManager) {
}

RenderSystem::~RenderSystem() {
    Destroy();
}

bool RenderSystem::Init() {
    m_IsDestroyed = false;
    assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
    p_SWRenderer_ = std::make_unique<SWRenderer>();
    auto& fbResolution = p_Config_->renderer.resolution;
    assert(fbResolution.x > 0 && fbResolution.y > 0 && "Tried to initialize renderers with invalid resolution");

    if (!p_SWRenderer_->Init(fbResolution.x, fbResolution.y)) {
        LOGE("Failed to initialize SWRenderer");
        return false;
    }
#if !defined(__EMSCRIPTEN__)
    StartSoftwareWorker();
#endif

    return true;
}

void RenderSystem::BeforeFrame(const Color& clearColor) {
    assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
    m_SoftwareClearColor = clearColor;
}

bool RenderSystem::PollSoftwareFrame() {
#if !defined(__EMSCRIPTEN__)
    PresentCompletedSoftwareFrame();
#endif
    return m_PresentedSoftwareFrame != nullptr;
}

std::shared_ptr<const RenderPacket> RenderSystem::BuildRenderPacket(const std::shared_ptr<Scene>& scene,
                                                                    const Camera* camera) {
    assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
    auto mutablePacket = std::make_shared<RenderPacket>();
    RenderPacket& packet = *mutablePacket;
    packet.configSnapshot = *p_Config_;
    packet.clearColor = p_Config_->renderer.clearColor;
    packet.dataRevision = m_FrameDataRevision;
    packet.sceneResourceRevision = m_SceneResourceRevision;
    packet.textureResourceRevision = m_TextureResourceRevision;
    if (!scene) {
        return mutablePacket;
    }

    const auto& currentMaterial = m_MaterialManager.GetCurrentMaterial();

    packet.hasScene = true;
    if (camera != nullptr) {
        packet.camera = *camera;
    }
    scene->BuildLightSnapshots(packet.lights);

    packet.materials.reserve(1);
    const FrameMaterialId sharedMaterialId = static_cast<FrameMaterialId>(packet.materials.size());
    packet.materials.push_back(CaptureFrameMaterialState(currentMaterial));

    const Texture* overrideTexture =
        currentMaterial.texture.has_value() && currentMaterial.texture->HasCpuPixels() ? &*currentMaterial.texture : nullptr;
    const auto captureTextureId = [&](const Texture* texture) -> FrameTextureId {
        if (texture == nullptr || !texture->HasCpuPixels()) {
            return kInvalidFrameTextureId;
        }

        std::shared_ptr<const Texture> textureSnapshot = GetOrCreateRenderTextureSnapshot(*texture);
        if (!textureSnapshot) {
            return kInvalidFrameTextureId;
        }

        const FrameTextureId textureId = static_cast<FrameTextureId>(packet.textures.size());
        packet.textures.push_back(std::move(textureSnapshot));
        return textureId;
    };
    const FrameTextureId overrideTextureId = captureTextureId(overrideTexture);

    const auto& visibleModels = scene->GetVisibleModels();
    size_t renderItemCount = 0;
    for (int modelIx : visibleModels) {
        if (modelIx < 0 || static_cast<size_t>(modelIx) >= scene->GetModelCount()) {
            continue;
        }
        renderItemCount += scene->GetModel(static_cast<size_t>(modelIx)).GetMeshCount();
    }
    packet.textures.reserve(std::max<size_t>(packet.textures.capacity(), std::min(renderItemCount, size_t{8})));
    packet.items.reserve(renderItemCount);
    std::unordered_map<const Texture*, FrameTextureId> textureIds;
    if (overrideTexture != nullptr && overrideTextureId != kInvalidFrameTextureId) {
        textureIds.emplace(overrideTexture, overrideTextureId);
    }
    const auto getTextureId = [&](const Texture* texture) -> FrameTextureId {
        if (texture == nullptr || !texture->HasCpuPixels()) {
            return kInvalidFrameTextureId;
        }
        auto it = textureIds.find(texture);
        if (it != textureIds.end()) {
            return it->second;
        }
        const FrameTextureId textureId = captureTextureId(texture);
        if (textureId != kInvalidFrameTextureId) {
            textureIds.emplace(texture, textureId);
        }
        return textureId;
    };

    for (int modelIx : visibleModels) {
        if (modelIx < 0 || static_cast<size_t>(modelIx) >= scene->GetModelCount()) {
            continue;
        }

        const Model& model = scene->GetModel(static_cast<size_t>(modelIx));
        for (size_t meshIx = 0; meshIx < model.GetMeshCount(); meshIx++) {
            const Mesh& mesh = model.GetMesh(meshIx);
            if (mesh.GetVertices().empty() || mesh.GetIndices().empty()) {
                continue;
            }

            std::shared_ptr<const RenderMeshSnapshot> meshSnapshot = GetOrCreateRenderMeshSnapshot(mesh);
            if (!meshSnapshot) {
                continue;
            }

            RenderItem item{};
            item.mesh = std::move(meshSnapshot);
            item.worldTransform = model.GetWorldTransform();
            item.materialId = sharedMaterialId;
            item.textureId =
                overrideTextureId != kInvalidFrameTextureId
                    ? overrideTextureId
                    : getTextureId(mesh.GetPrimaryTexture());
            packet.items.push_back(item);
        }
    }

    return mutablePacket;
}

std::shared_ptr<const CpuFrame> RenderSystem::PrepareFrame(const std::shared_ptr<const RenderPacket>& packet) {
    assert(packet && packet->hasScene && "Render called with empty render packet");
    assert(p_Stats_ != nullptr && "RenderSystem requires stats");
    const auto renderSystemStart = TimingClock::now();
    p_Stats_->Reset();

    if (packet->configSnapshot.renderer.selectedRenderer == Config::RendererType::SOFTWARE) {
        p_Stats_->lastGlRenderNs.store(0, std::memory_order_relaxed);
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

    p_Stats_->lastSoftwarePacketCopyNs.store(0, std::memory_order_relaxed);
    p_Stats_->lastRenderSystemNs.store(ElapsedNanoseconds(renderSystemStart), std::memory_order_relaxed);
    return nullptr;
}

void RenderSystem::Resize(const glm::ivec2& resolution) {
    assert(resolution.x > 0 && resolution.y > 0 && "Tried to resize renderer with invalid resolution");
    assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
    p_Config_->renderer.resolution = resolution;

#if !defined(__EMSCRIPTEN__)
    StopSoftwareWorker();
#endif

    LOGI("Resizing output image to %d x %d", resolution.x, resolution.y);
    p_SWRenderer_->Resize(resolution.x, resolution.y);

#if !defined(__EMSCRIPTEN__)
    ClearSoftwareWorkerFrameState();
    StartSoftwareWorker();
#endif
}

void RenderSystem::Destroy() {
    if (m_IsDestroyed) {
        return;
    }
    m_IsDestroyed = true;

    StopSoftwareWorker();
    ClearRenderResourceSnapshots();
    if (p_SWRenderer_) {
        p_SWRenderer_->Destroy();
    }
}

void RenderSystem::OnLoadScene(const SceneLoadEvent& e) {
    (void)e;
    ClearRenderResourceSnapshots();
    ++m_FrameDataRevision;
    ++m_SceneResourceRevision;
    ++m_TextureResourceRevision;
}

void RenderSystem::OnResetScene() {
    ClearRenderResourceSnapshots();
    ++m_FrameDataRevision;
    ++m_SceneResourceRevision;
    ++m_TextureResourceRevision;
}

void RenderSystem::OnSceneMutated() {
    ++m_FrameDataRevision;
}

void RenderSystem::OnTextureMutated() {
    m_RenderTextureSnapshots.clear();
    ++m_FrameDataRevision;
    ++m_TextureResourceRevision;
}

void RenderSystem::ClearSoftwareWorkerFrameState() {
#if !defined(__EMSCRIPTEN__)
    std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
    m_PendingSoftwareJob.reset();
    m_CompletedSoftwareFrames.clear();
    m_SoftwareWorkerBusy = false;
#endif
    m_PresentedSoftwareFrame.reset();
    if (p_Stats_) {
        p_Stats_->lastSoftwareFramePresentedNs.store(0, std::memory_order_relaxed);
        p_Stats_->lastSoftwareFramePresentIntervalNs.store(0, std::memory_order_relaxed);
    }
}

std::shared_ptr<const RenderMeshSnapshot> RenderSystem::GetOrCreateRenderMeshSnapshot(const Mesh& mesh) {
    auto it = m_RenderMeshSnapshots.find(&mesh);
    if (it != m_RenderMeshSnapshots.end()) {
        return it->second;
    }

    auto snapshot = std::make_shared<RenderMeshSnapshot>();
    snapshot->vertices = mesh.GetVertices();
    snapshot->indices = mesh.GetIndices();
    std::shared_ptr<const RenderMeshSnapshot> immutableSnapshot = std::move(snapshot);
    m_RenderMeshSnapshots.emplace(&mesh, immutableSnapshot);
    return immutableSnapshot;
}

std::shared_ptr<const Texture> RenderSystem::GetOrCreateRenderTextureSnapshot(const Texture& texture) {
    if (!texture.HasCpuPixels()) {
        return nullptr;
    }

    auto it = m_RenderTextureSnapshots.find(&texture);
    if (it != m_RenderTextureSnapshots.end() && it->second.revision == texture.GetRevision()) {
        return it->second.texture;
    }

    auto snapshot = std::make_shared<Texture>(texture.CloneCpuOnly());
    std::shared_ptr<const Texture> immutableSnapshot = std::move(snapshot);
    m_RenderTextureSnapshots[&texture] = RenderTextureSnapshotCacheEntry{texture.GetRevision(), immutableSnapshot};
    return immutableSnapshot;
}

void RenderSystem::ClearRenderResourceSnapshots() {
    m_RenderMeshSnapshots.clear();
    m_RenderTextureSnapshots.clear();
}

void RenderSystem::StartSoftwareWorker() {
#if !defined(__EMSCRIPTEN__)
    if (m_SoftwareWorkerThread.joinable()) {
        return;
    }
    m_SoftwareWorkerStopRequested = false;
    m_SoftwareWorkerThread = std::thread(&RenderSystem::SoftwareWorkerLoop, this);
#endif
}

void RenderSystem::StopSoftwareWorker() {
#if !defined(__EMSCRIPTEN__)
    if (!m_SoftwareWorkerThread.joinable()) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
        m_SoftwareWorkerStopRequested = true;
        m_PendingSoftwareJob.reset();
    }
    m_SoftwareWorkerCv.notify_one();
    m_SoftwareWorkerThread.join();
    {
        std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
        m_SoftwareWorkerStopRequested = false;
        m_PendingSoftwareJob.reset();
        m_CompletedSoftwareFrames.clear();
        m_SoftwareWorkerBusy = false;
    }
#endif
}

void RenderSystem::SubmitSoftwareJob(const std::shared_ptr<const RenderPacket>& packet) {
#if !defined(__EMSCRIPTEN__)
    if (!packet || !packet->hasScene) {
        return;
    }
    assert(p_Stats_ != nullptr && "RenderSystem requires stats");

    {
        std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
        if (m_PendingSoftwareJob.has_value() || m_SoftwareWorkerBusy) {
            p_Stats_->swJobsSkippedBusy.fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }

    SoftwareRenderJob job{};
    const auto softwarePacketCopyStart = TimingClock::now();
    job.packet = packet;
    p_Stats_->lastSoftwarePacketCopyNs.store(ElapsedNanoseconds(softwarePacketCopyStart), std::memory_order_relaxed);
    if (!job.packet || !job.packet->hasScene) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
        job.jobId = ++m_NextSoftwareJobId;
        if (m_PendingSoftwareJob.has_value()) {
            p_Stats_->swJobsDroppedPending.fetch_add(1, std::memory_order_relaxed);
        }
        m_PendingSoftwareJob = std::move(job);
    }

    p_Stats_->swJobsSubmitted.fetch_add(1, std::memory_order_relaxed);
    m_SoftwareWorkerCv.notify_one();
#endif
}

void RenderSystem::PresentCompletedSoftwareFrame() {
#if !defined(__EMSCRIPTEN__)
    assert(p_Stats_ != nullptr && "RenderSystem requires stats");

    std::shared_ptr<CpuFrame> completedFrame;
    size_t droppedReadyFrames = 0;
    bool hasFrame = false;
    size_t width = 0;
    size_t height = 0;
    {
        std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
        while (!m_CompletedSoftwareFrames.empty() &&
               m_CompletedSoftwareFrames.back()->dataRevision < m_FrameDataRevision) {
            m_CompletedSoftwareFrames.pop_back();
            droppedReadyFrames++;
        }
        if (m_CompletedSoftwareFrames.empty()) {
            return;
        }
        completedFrame = std::move(m_CompletedSoftwareFrames.back());
        droppedReadyFrames = m_CompletedSoftwareFrames.size() - 1;
        m_CompletedSoftwareFrames.clear();
        hasFrame = true;
        width = completedFrame->width;
        height = completedFrame->height;
    }
    if (!hasFrame || !completedFrame || completedFrame->pixels.empty() || width == 0 || height == 0) {
        return;
    }

    if (completedFrame->pitch == 0) {
        completedFrame->pitch = completedFrame->width * sizeof(Pixel);
    }
    m_PresentedSoftwareFrame = std::move(completedFrame);

    if (droppedReadyFrames > 0) {
        p_Stats_->swFramesDroppedReady.fetch_add(droppedReadyFrames, std::memory_order_relaxed);
    }
    RecordSoftwareFramePresented();
#endif
}

void RenderSystem::RecordSoftwareFramePresented() {
    assert(p_Stats_ != nullptr && "RenderSystem requires stats");
    const uint64_t nowNs = ClockNanoseconds();
    const uint64_t previousNs = p_Stats_->lastSoftwareFramePresentedNs.exchange(nowNs, std::memory_order_relaxed);
    if (previousNs != 0 && nowNs > previousNs) {
        p_Stats_->lastSoftwareFramePresentIntervalNs.store(nowNs - previousNs, std::memory_order_relaxed);
    }
    p_Stats_->swFramesPresented.fetch_add(1, std::memory_order_relaxed);
}

void RenderSystem::SoftwareWorkerLoop() {
#if !defined(__EMSCRIPTEN__)
    assert(p_Stats_ != nullptr && "RenderSystem requires stats");

    while (true) {
        SoftwareRenderJob job{};
        {
            std::unique_lock<std::mutex> lock(m_SoftwareWorkerMutex);
            m_SoftwareWorkerCv.wait(lock, [this]() { return m_SoftwareWorkerStopRequested || m_PendingSoftwareJob.has_value(); });
            if (m_SoftwareWorkerStopRequested) {
                return;
            }
            job = std::move(*m_PendingSoftwareJob);
            m_PendingSoftwareJob.reset();
            m_SoftwareWorkerBusy = true;
        }

        const auto workerRenderStart = TimingClock::now();
        p_SWRenderer_->RenderFrame(*job.packet);
        p_Stats_->lastSoftwareWorkerRenderNs.store(ElapsedNanoseconds(workerRenderStart), std::memory_order_relaxed);

        const auto workerCopyStart = TimingClock::now();
        const auto& buffer = p_SWRenderer_->GetFrameBuffer();
        auto finishedFrame = std::make_shared<CpuFrame>();
        finishedFrame->width = buffer.width;
        finishedFrame->height = buffer.height;
        finishedFrame->pitch = buffer.pitch;
        finishedFrame->frameId = job.jobId;
        finishedFrame->dataRevision = job.packet->dataRevision;
        finishedFrame->pixels.resize(buffer.GetCount());
        if (!finishedFrame->pixels.empty()) {
            std::memcpy(finishedFrame->pixels.data(), buffer.data, finishedFrame->pixels.size() * sizeof(Pixel));
        }
        p_Stats_->lastSoftwareWorkerCopyNs.store(ElapsedNanoseconds(workerCopyStart), std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            if (m_CompletedSoftwareFrames.size() >= kMaxBufferedSoftwareFrames) {
                m_CompletedSoftwareFrames.pop_front();
                p_Stats_->swFramesDroppedReady.fetch_add(1, std::memory_order_relaxed);
            }
            m_CompletedSoftwareFrames.push_back(std::move(finishedFrame));
            m_SoftwareWorkerBusy = false;
        }
        p_Stats_->swJobsCompleted.fetch_add(1, std::memory_order_relaxed);
    }
#endif
}

void RenderSystem::RenderSoftwareSync(const RenderPacket& packet) {
    p_Stats_->lastSoftwarePacketCopyNs.store(0, std::memory_order_relaxed);

    const auto workerRenderStart = TimingClock::now();
    p_SWRenderer_->RenderFrame(packet);
    p_Stats_->lastSoftwareWorkerRenderNs.store(ElapsedNanoseconds(workerRenderStart), std::memory_order_relaxed);

    const auto workerCopyStart = TimingClock::now();
    const auto& buffer = p_SWRenderer_->GetFrameBuffer();
    StoreSoftwareFrame(buffer);
    p_Stats_->lastSoftwareWorkerCopyNs.store(ElapsedNanoseconds(workerCopyStart), std::memory_order_relaxed);
    RecordSoftwareFramePresented();
}

void RenderSystem::StoreSoftwareFrame(const Buffer<Pixel>& buffer) {
    auto frame = std::make_shared<CpuFrame>();
    frame->width = buffer.width;
    frame->height = buffer.height;
    frame->pitch = buffer.pitch;
    frame->pixels.resize(buffer.GetCount());
    if (!frame->pixels.empty()) {
        std::memcpy(
            frame->pixels.data(),
            buffer.data,
            frame->pixels.size() * sizeof(Pixel));
    }
    m_PresentedSoftwareFrame = std::move(frame);
}
} // namespace RetroRenderer
