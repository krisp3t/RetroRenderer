#include "RenderSystem.h"
#include "../Scene/MaterialManager.h"
#include "GLFramePresenter.h"
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#include "GLES/GLESRenderer.h"
#else
#include "OpenGL/GLRenderer.h"
#endif
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
    state.shaderHandle = currentMaterial.shaderProgram.handle;
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
    m_GLFramePresenter = std::make_unique<GLFramePresenter>();
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    p_GLRenderer_ = std::make_unique<GLESRenderer>();
#else
    p_GLRenderer_ = std::make_unique<GLRenderer>();
#endif

    p_activeRenderer_ = p_GLRenderer_.get();
    auto& fbResolution = p_Config_->renderer.resolution;
    assert(fbResolution.x > 0 && fbResolution.y > 0 && "Tried to initialize renderers with invalid resolution");

    if (!m_GLFramePresenter->Init(
            fbResolution.x, fbResolution.y, p_Config_->renderer.nearestNeighborPresentation)) {
        LOGE("Failed to initialize GL frame presenter");
        return false;
    }

    if (!p_SWRenderer_->Init(fbResolution.x, fbResolution.y)) {
        LOGE("Failed to initialize SWRenderer");
        return false;
    }
    if (!p_GLRenderer_->Init(m_GLFramePresenter->GetTextureHandle(), fbResolution.x, fbResolution.y)) {
        LOGE("Failed to initialize GLRenderer");
        return false;
    }

#if !defined(__EMSCRIPTEN__)
    StartSoftwareWorker();
#endif

    return true;
}

void RenderSystem::BeforeFrame(const Color& clearColor) {
    assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
    switch (p_Config_->renderer.selectedRenderer) {
    case Config::RendererType::SOFTWARE:
        p_activeRenderer_ = p_SWRenderer_.get();
        break;
    case Config::RendererType::GL:
        p_activeRenderer_ = p_GLRenderer_.get();
        break;
    default:
        LOGE("Renderer type %d not implemented!", p_Config_->renderer.selectedRenderer);
        return;
    }

    assert(p_activeRenderer_ != nullptr && "Active renderer is null");
    m_SoftwareClearColor = clearColor;
}

const RenderPacket& RenderSystem::BuildRenderPacket(const std::shared_ptr<Scene>& scene, const Camera& camera) {
    RenderPacket& packet = m_RenderPacketScratch;
    packet.hasScene = false;
    packet.lights.clear();
    packet.materials.clear();
    packet.textures.clear();
    packet.items.clear();
    if (!scene) {
        return packet;
    }

    assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
    const auto& currentMaterial = m_MaterialManager.GetCurrentMaterial();

    packet.hasScene = true;
    packet.camera = camera;
    scene->BuildLightSnapshots(packet.lights);
    packet.configSnapshot = *p_Config_;
    packet.clearColor = p_Config_->renderer.clearColor;
    packet.dataRevision = m_FrameDataRevision;

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

    return packet;
}

RenderOutput RenderSystem::Render(const RenderPacket& packet) {
    assert(packet.hasScene && "Render called with empty render packet");
    assert(p_Stats_ != nullptr && "RenderSystem requires stats");
    const auto renderSystemStart = TimingClock::now();
    p_Stats_->Reset();

    if (p_activeRenderer_ == p_SWRenderer_.get()) {
        p_Stats_->lastGlRenderNs.store(0, std::memory_order_relaxed);
#if defined(__EMSCRIPTEN__)
        RenderOutput output = RenderSoftwareSync(packet);
        p_Stats_->lastRenderSystemNs.store(ElapsedNanoseconds(renderSystemStart), std::memory_order_relaxed);
        return output;
#else
        SubmitSoftwareJob(packet);
        PresentCompletedSoftwareFrame();
        RenderOutput output = MakeSoftwareRenderOutput();
        p_Stats_->lastRenderSystemNs.store(ElapsedNanoseconds(renderSystemStart), std::memory_order_relaxed);
        return output;
#endif
    }

    p_Stats_->lastSoftwarePacketCopyNs.store(0, std::memory_order_relaxed);
    const auto glRenderStart = TimingClock::now();
    p_activeRenderer_->RenderFrame(packet);
    p_Stats_->lastGlRenderNs.store(ElapsedNanoseconds(glRenderStart), std::memory_order_relaxed);
    RenderOutput output = RenderOutput::Texture(m_GLFramePresenter->GetTextureHandle(), RenderOutputOrigin::BottomLeft);
    p_Stats_->lastRenderSystemNs.store(ElapsedNanoseconds(renderSystemStart), std::memory_order_relaxed);
    return output;
}

void RenderSystem::Resize(const glm::ivec2& resolution) {
    assert(resolution.x > 0 && resolution.y > 0 && "Tried to resize renderer with invalid resolution");
    assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
    p_Config_->renderer.resolution = resolution;

#if !defined(__EMSCRIPTEN__)
    StopSoftwareWorker();
#endif

    m_GLFramePresenter->Resize(resolution.x, resolution.y, p_Config_->renderer.nearestNeighborPresentation);
    LOGI("Resizing output image to %d x %d", resolution.x, resolution.y);
    p_SWRenderer_->Resize(resolution.x, resolution.y);
    p_GLRenderer_->Resize(m_GLFramePresenter->GetTextureHandle(), resolution.x, resolution.y);

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
    if (p_GLRenderer_) {
        p_GLRenderer_->Destroy();
    }
    if (p_SWRenderer_) {
        p_SWRenderer_->Destroy();
    }
    if (m_GLFramePresenter) {
        m_GLFramePresenter->Destroy();
    }
}

void RenderSystem::OnLoadScene(const SceneLoadEvent& e) {
    (void)e;
    p_GLRenderer_->InvalidateSceneResources();
    ClearRenderResourceSnapshots();
    ++m_FrameDataRevision;
}

void RenderSystem::OnResetScene() {
    p_GLRenderer_->InvalidateSceneResources();
    ClearRenderResourceSnapshots();
    ++m_FrameDataRevision;
}

void RenderSystem::OnSceneMutated() {
    ++m_FrameDataRevision;
}

void RenderSystem::OnTextureMutated() {
    p_GLRenderer_->InvalidateTextureResources();
    m_RenderTextureSnapshots.clear();
    ++m_FrameDataRevision;
}

ShaderHandle RenderSystem::CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) {
    return p_GLRenderer_->CompileShaders(vertexCode, fragmentCode);
}

void RenderSystem::ClearSoftwareWorkerFrameState() {
#if !defined(__EMSCRIPTEN__)
    std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
    m_PendingSoftwareJob.reset();
    m_CompletedSoftwareFrames.clear();
    m_SoftwareWorkerBusy = false;
#endif
    m_PresentedSoftwareFrame = {};
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

void RenderSystem::SubmitSoftwareJob(const RenderPacket& packet) {
#if !defined(__EMSCRIPTEN__)
    if (!packet.hasScene) {
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
    if (!job.packet.hasScene) {
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

    SoftwareCompletedFrame completedFrame;
    size_t droppedReadyFrames = 0;
    bool hasFrame = false;
    size_t width = 0;
    size_t height = 0;
    {
        std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
        while (!m_CompletedSoftwareFrames.empty() &&
               m_CompletedSoftwareFrames.back().dataRevision < m_FrameDataRevision) {
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
        width = completedFrame.width;
        height = completedFrame.height;
    }
    if (!hasFrame || completedFrame.pixels.empty() || width == 0 || height == 0) {
        return;
    }

    if (completedFrame.pitch == 0) {
        completedFrame.pitch = completedFrame.width * sizeof(Pixel);
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
        p_SWRenderer_->RenderFrame(job.packet);
        p_Stats_->lastSoftwareWorkerRenderNs.store(ElapsedNanoseconds(workerRenderStart), std::memory_order_relaxed);

        const auto workerCopyStart = TimingClock::now();
        const auto& buffer = p_SWRenderer_->GetFrameBuffer();
        SoftwareCompletedFrame finishedFrame{};
        finishedFrame.width = buffer.width;
        finishedFrame.height = buffer.height;
        finishedFrame.pitch = buffer.pitch;
        finishedFrame.jobId = job.jobId;
        finishedFrame.dataRevision = job.packet.dataRevision;
        finishedFrame.pixels.resize(buffer.GetCount());
        if (!finishedFrame.pixels.empty()) {
            std::memcpy(finishedFrame.pixels.data(), buffer.data, finishedFrame.pixels.size() * sizeof(Pixel));
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

RenderOutput RenderSystem::RenderSoftwareSync(const RenderPacket& packet) {
    p_Stats_->lastSoftwarePacketCopyNs.store(0, std::memory_order_relaxed);

    const auto workerRenderStart = TimingClock::now();
    p_SWRenderer_->RenderFrame(packet);
    p_Stats_->lastSoftwareWorkerRenderNs.store(ElapsedNanoseconds(workerRenderStart), std::memory_order_relaxed);

    const auto workerCopyStart = TimingClock::now();
    const auto& buffer = p_SWRenderer_->GetFrameBuffer();
    StoreSoftwareFrame(buffer);
    p_Stats_->lastSoftwareWorkerCopyNs.store(ElapsedNanoseconds(workerCopyStart), std::memory_order_relaxed);
    RecordSoftwareFramePresented();
    return MakeSoftwareRenderOutput();
}

RenderOutput RenderSystem::MakeSoftwareRenderOutput() const {
    return RenderOutput::Pixels(
        m_PresentedSoftwareFrame.pixels.data(),
        m_PresentedSoftwareFrame.width,
        m_PresentedSoftwareFrame.height,
        m_PresentedSoftwareFrame.pitch,
        RenderOutputOrigin::TopLeft);
}

void RenderSystem::StoreSoftwareFrame(const Buffer<Pixel>& buffer) {
    m_PresentedSoftwareFrame = {};
    m_PresentedSoftwareFrame.width = buffer.width;
    m_PresentedSoftwareFrame.height = buffer.height;
    m_PresentedSoftwareFrame.pitch = buffer.pitch;
    m_PresentedSoftwareFrame.pixels.resize(buffer.GetCount());
    if (!m_PresentedSoftwareFrame.pixels.empty()) {
        std::memcpy(
            m_PresentedSoftwareFrame.pixels.data(),
            buffer.data,
            m_PresentedSoftwareFrame.pixels.size() * sizeof(Pixel));
    }
}
} // namespace RetroRenderer
