#include "RenderSystem.h"
#include "../Scene/MaterialManager.h"
#include "GLFramePresenter.h"
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#include "GLES/GLESRenderer.h"
#else
#include "OpenGL/GLRenderer.h"
#endif
#include <KrisLogger/Logger.h>
#include <cassert>
#include <cstring>

namespace RetroRenderer {
namespace {
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
    if (currentMaterial.texture.has_value() && currentMaterial.texture->HasCpuPixels()) {
        state.textureOverride = &*currentMaterial.texture;
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

FrameSnapshot RenderSystem::BuildFrameSnapshot(const std::shared_ptr<Scene>& scene, const Camera& camera) const {
    FrameSnapshot frame{};
    if (!scene) {
        return frame;
    }

    assert(p_Config_ != nullptr && "RenderSystem requires a config instance");
    const auto& currentMaterial = m_MaterialManager.GetCurrentMaterial();

    frame.scene = scene;
    frame.camera = camera;
    frame.lights = scene->BuildLightSnapshots();
    frame.items.clear();
    frame.configSnapshot = *p_Config_;
    frame.clearColor = p_Config_->renderer.clearColor;
    frame.materialState = CaptureFrameMaterialState(currentMaterial);

    const auto& visibleModels = scene->GetVisibleModels();
    size_t renderItemCount = 0;
    for (int modelIx : visibleModels) {
        if (modelIx < 0 || static_cast<size_t>(modelIx) >= scene->GetModelCount()) {
            continue;
        }
        renderItemCount += scene->GetModel(static_cast<size_t>(modelIx)).GetMeshCount();
    }
    frame.items.reserve(renderItemCount);

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

            RenderItem item{};
            item.modelIndex = modelIx;
            item.meshIndex = static_cast<int>(meshIx);
            item.worldTransform = model.GetWorldTransform();
            frame.items.push_back(item);
        }
    }

    return frame;
}

RenderOutput RenderSystem::Render(const FrameSnapshot& frame) {
    assert(frame.scene != nullptr && "Render called with null scene");
    assert(p_Stats_ != nullptr && "RenderSystem requires stats");
    p_Stats_->Reset();

    if (p_activeRenderer_ == p_SWRenderer_.get()) {
#if defined(__EMSCRIPTEN__)
        return RenderSoftwareSync(frame);
#else
        SubmitSoftwareJob(frame);
        PresentCompletedSoftwareFrame();
        return MakeSoftwareRenderOutput();
#endif
    }

    p_activeRenderer_->RenderFrame(frame);
    return RenderOutput::Texture(m_GLFramePresenter->GetTextureHandle(), RenderOutputOrigin::BottomLeft);
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
    SyncSoftwareWorkerForRenderDataMutation();
}

void RenderSystem::OnResetScene() {
    p_GLRenderer_->InvalidateSceneResources();
    SyncSoftwareWorkerForRenderDataMutation();
}

void RenderSystem::OnSceneMutated() {
    SyncSoftwareWorkerForRenderDataMutation();
}

void RenderSystem::OnTextureMutated() {
    p_GLRenderer_->InvalidateTextureResources();
    SyncSoftwareWorkerForRenderDataMutation();
}

ShaderHandle RenderSystem::CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) {
    return p_GLRenderer_->CompileShaders(vertexCode, fragmentCode);
}

void RenderSystem::ClearSoftwareWorkerFrameState() {
#if !defined(__EMSCRIPTEN__)
    std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
    m_PendingSoftwareJob.reset();
    m_CompletedSoftwareFrames.clear();
#endif
    m_PresentedSoftwareFrame = {};
}

void RenderSystem::SyncSoftwareWorkerForRenderDataMutation() {
#if !defined(__EMSCRIPTEN__)
    StopSoftwareWorker();
    ClearSoftwareWorkerFrameState();
    if (p_SWRenderer_) {
        p_SWRenderer_->BeforeFrame(m_SoftwareClearColor);
        const auto& buffer = p_SWRenderer_->GetFrameBuffer();
        StoreSoftwareFrame(buffer);
    }
    StartSoftwareWorker();
#endif
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
    }
#endif
}

void RenderSystem::SubmitSoftwareJob(const FrameSnapshot& frame) {
#if !defined(__EMSCRIPTEN__)
    if (!frame.scene) {
        return;
    }
    assert(p_Stats_ != nullptr && "RenderSystem requires stats");

    SoftwareRenderJob job{};
    job.frame = frame;
    if (frame.materialState.textureOverride != nullptr) {
        job.textureOverride = frame.materialState.textureOverride->CloneCpuOnly();
        job.frame.materialState.textureOverride = &*job.textureOverride;
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
    p_Stats_->swFramesPresented.fetch_add(1, std::memory_order_relaxed);
#endif
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
        }

        p_SWRenderer_->RenderFrame(job.frame);
        const auto& buffer = p_SWRenderer_->GetFrameBuffer();
        SoftwareCompletedFrame finishedFrame{};
        finishedFrame.width = buffer.width;
        finishedFrame.height = buffer.height;
        finishedFrame.pitch = buffer.pitch;
        finishedFrame.jobId = job.jobId;
        finishedFrame.pixels.resize(buffer.GetCount());
        if (!finishedFrame.pixels.empty()) {
            std::memcpy(finishedFrame.pixels.data(), buffer.data, finishedFrame.pixels.size() * sizeof(Pixel));
        }

        {
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            if (m_CompletedSoftwareFrames.size() >= kMaxBufferedSoftwareFrames) {
                m_CompletedSoftwareFrames.pop_front();
                p_Stats_->swFramesDroppedReady.fetch_add(1, std::memory_order_relaxed);
            }
            m_CompletedSoftwareFrames.push_back(std::move(finishedFrame));
        }
        p_Stats_->swJobsCompleted.fetch_add(1, std::memory_order_relaxed);
    }
#endif
}

RenderOutput RenderSystem::RenderSoftwareSync(const FrameSnapshot& frame) {
    p_SWRenderer_->RenderFrame(frame);
    const auto& buffer = p_SWRenderer_->GetFrameBuffer();
    StoreSoftwareFrame(buffer);
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
