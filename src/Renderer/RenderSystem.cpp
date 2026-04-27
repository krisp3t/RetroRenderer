#include "RenderSystem.h"
#include "../Engine.h"
#include "../Scene/MaterialManager.h"
#include <KrisLogger/Logger.h>
#include <cassert>
#include <cstring>

namespace RetroRenderer {
namespace {
std::optional<Texture> CaptureSoftwareFallbackTexture() {
    auto& currentMaterial = Engine::Get().GetMaterialManager().GetCurrentMaterial();
    if (!currentMaterial.texture.has_value() || !currentMaterial.texture->HasCpuPixels()) {
        return std::nullopt;
    }
    return currentMaterial.texture->CloneCpuOnly();
}

SoftwareMaterialState CaptureSoftwareMaterialState() {
    const auto& currentMaterial = Engine::Get().GetMaterialManager().GetCurrentMaterial();
    SoftwareMaterialState state{};
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

RenderSystem::~RenderSystem() {
    Destroy();
}

bool RenderSystem::Init() {
    m_IsDestroyed = false;
    auto const& p_config = Engine::Get().GetConfig();
    p_SWRenderer_ = std::make_unique<SWRenderer>();
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    p_GLRenderer_ = std::make_unique<GLESRenderer>();
#else
    p_GLRenderer_ = std::make_unique<GLRenderer>();
#endif

    p_activeRenderer_ = p_GLRenderer_.get();
    auto& fbResolution = p_config->renderer.resolution;
    assert(fbResolution.x > 0 && fbResolution.y > 0 && "Tried to initialize renderers with invalid resolution");

    if (!m_SWFramePresenter.Init(fbResolution.x, fbResolution.y, p_config->renderer.nearestNeighborPresentation)) {
        LOGE("Failed to initialize software frame presenter");
        return false;
    }
    if (!m_GLFramePresenter.Init(fbResolution.x, fbResolution.y, p_config->renderer.nearestNeighborPresentation)) {
        LOGE("Failed to initialize GL frame presenter");
        return false;
    }

    if (!p_SWRenderer_->Init(fbResolution.x, fbResolution.y)) {
        LOGE("Failed to initialize SWRenderer");
        return false;
    }
    if (!p_GLRenderer_->Init(m_GLFramePresenter.GetTextureHandle(), fbResolution.x, fbResolution.y)) {
        LOGE("Failed to initialize GLRenderer");
        return false;
    }

#if !defined(__EMSCRIPTEN__)
    StartSoftwareWorker();
#endif

    return true;
}

void RenderSystem::BeforeFrame(const Color& clearColor) {
    auto& p_config = Engine::Get().GetConfig();
    switch (p_config->renderer.selectedRenderer) {
    case Config::RendererType::SOFTWARE:
        p_activeRenderer_ = p_SWRenderer_.get();
        break;
    case Config::RendererType::GL:
        p_activeRenderer_ = p_GLRenderer_.get();
        break;
    default:
        LOGE("Renderer type %d not implemented!", p_config->renderer.selectedRenderer);
        return;
    }
    assert(p_activeRenderer_ != nullptr && "Active renderer is null");
    glViewport(0, 0, p_config->renderer.resolution.x, p_config->renderer.resolution.y);
    if (p_activeRenderer_ == p_SWRenderer_.get()) {
        m_SoftwareClearColor = clearColor;
#if defined(__EMSCRIPTEN__)
        // Emscripten path remains single-threaded.
        p_activeRenderer_->BeforeFrame(clearColor);
#endif
        return;
    }
    p_activeRenderer_->BeforeFrame(clearColor);
}

std::vector<int>& RenderSystem::BuildRenderQueue(Scene& scene, const Camera& camera) {
    (void)camera;
    return scene.GetVisibleModels(); // TODO: split into meshes?
}

GLuint RenderSystem::Render(const std::shared_ptr<Scene>& scene, const Camera& camera, const std::vector<int>& renderQueue) {
    assert(scene != nullptr && "Render called with null scene");
    auto const& p_config = Engine::Get().GetConfig();
    auto const& p_stats = Engine::Get().GetStats();
    assert(p_stats != nullptr && "Stats not initialized!");
    p_stats->Reset();

    if (p_activeRenderer_ == p_SWRenderer_.get()) {
#if defined(__EMSCRIPTEN__)
        return RenderSoftwareSync(scene, camera, renderQueue);
#else
        SubmitSoftwareJob(scene, camera, renderQueue);
        UploadSoftwareFrameToTexture();
        return m_SWFramePresenter.GetTextureHandle();
#endif
    }

    const std::vector<LightSnapshot> lightSnapshots = scene->BuildLightSnapshots();
    p_activeRenderer_->SetActiveCamera(camera);
    p_activeRenderer_->SetSceneLights(lightSnapshots);

    if (p_config->environment.showSkybox) {
        p_activeRenderer_->DrawSkybox();
    }
    auto& models = scene->m_Models;
    // LOGD("Render queue size: %d", renderQueue.size());
    for (int modelIx : renderQueue) {
        if (modelIx < 0 || static_cast<size_t>(modelIx) >= models.size()) {
            continue;
        }
        const Model* model = &models[modelIx];
        assert(model != nullptr && "Model is null");
        if (!model->GetMeshes().empty()) {
            p_activeRenderer_->DrawTriangularMesh(model);
        }
    }
    if (p_config->environment.showGrid) {
        p_activeRenderer_->DrawGridGizmo();
    }
    p_activeRenderer_->EndFrame();
    return m_GLFramePresenter.GetTextureHandle();
}

void RenderSystem::Resize(const glm::ivec2& resolution) {
    assert(resolution.x > 0 && resolution.y > 0 && "Tried to resize renderer with invalid resolution");
    auto const& p_config = Engine::Get().GetConfig();
    p_config->renderer.resolution = resolution;

#if !defined(__EMSCRIPTEN__)
    StopSoftwareWorker();
#endif

    m_SWFramePresenter.Resize(resolution.x, resolution.y, p_config->renderer.nearestNeighborPresentation);
    m_GLFramePresenter.Resize(resolution.x, resolution.y, p_config->renderer.nearestNeighborPresentation);
    LOGI("Resizing output image to %d x %d", resolution.x, resolution.y);
    p_SWRenderer_->Resize(resolution.x, resolution.y);
    p_GLRenderer_->Resize(m_GLFramePresenter.GetTextureHandle(), resolution.x, resolution.y);

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
    m_SWFramePresenter.Destroy();
    m_GLFramePresenter.Destroy();
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

GLuint RenderSystem::GetTextureHandle(const Texture& texture) {
    return p_GLRenderer_->GetTextureHandle(texture);
}

ShaderHandle RenderSystem::CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) {
    return p_activeRenderer_->CompileShaders(vertexCode, fragmentCode);
}

void RenderSystem::ClearSoftwareWorkerFrameState() {
#if !defined(__EMSCRIPTEN__)
    std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
    m_PendingSoftwareJob.reset();
    m_CompletedSoftwareFrames.clear();
#endif
}

void RenderSystem::SyncSoftwareWorkerForRenderDataMutation() {
#if !defined(__EMSCRIPTEN__)
    StopSoftwareWorker();
    ClearSoftwareWorkerFrameState();
    if (p_SWRenderer_ && m_SWFramePresenter.GetTextureHandle() != 0) {
        p_SWRenderer_->BeforeFrame(m_SoftwareClearColor);
        const auto& buffer = p_SWRenderer_->GetFrameBuffer();
        m_SWFramePresenter.Upload(buffer);
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

void RenderSystem::SubmitSoftwareJob(const std::shared_ptr<Scene>& scene,
                                     const Camera& camera,
                                     const std::vector<int>& renderQueue) {
#if !defined(__EMSCRIPTEN__)
    if (!scene) {
        return;
    }
    auto const& p_stats = Engine::Get().GetStats();
    assert(p_stats != nullptr && "Stats not initialized!");

    SoftwareRenderJob job;
    job.scene = scene;
    job.camera = std::make_shared<Camera>(camera);
    job.lights = scene->BuildLightSnapshots();
    job.renderQueue = renderQueue;
    job.configSnapshot = *Engine::Get().GetConfig();
    job.materialState = CaptureSoftwareMaterialState();
    job.clearColor = m_SoftwareClearColor;
    job.fallbackTexture = CaptureSoftwareFallbackTexture();
    {
        std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
        job.jobId = ++m_NextSoftwareJobId;
        if (m_PendingSoftwareJob.has_value()) {
            p_stats->swJobsDroppedPending.fetch_add(1, std::memory_order_relaxed);
        }
        m_PendingSoftwareJob = std::move(job); // Keep only the latest job.
    }
    p_stats->swJobsSubmitted.fetch_add(1, std::memory_order_relaxed);
    m_SoftwareWorkerCv.notify_one();
#endif
}

void RenderSystem::UploadSoftwareFrameToTexture() {
#if !defined(__EMSCRIPTEN__)
    auto const& p_stats = Engine::Get().GetStats();
    assert(p_stats != nullptr && "Stats not initialized!");

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
        // Present newest finished frame and drop older stale ones.
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

    if (!m_SWFramePresenter.UploadPixels(completedFrame.pixels.data(), width, height)) {
        return;
    }

    if (droppedReadyFrames > 0) {
        p_stats->swFramesDroppedReady.fetch_add(droppedReadyFrames, std::memory_order_relaxed);
    }
    p_stats->swFramesUploaded.fetch_add(1, std::memory_order_relaxed);
#endif
}

void RenderSystem::SoftwareWorkerLoop() {
#if !defined(__EMSCRIPTEN__)
    auto const& p_stats = Engine::Get().GetStats();
    assert(p_stats != nullptr && "Stats not initialized!");

    while (true) {
        SoftwareRenderJob job;
        {
            std::unique_lock<std::mutex> lock(m_SoftwareWorkerMutex);
            m_SoftwareWorkerCv.wait(lock, [this]() { return m_SoftwareWorkerStopRequested || m_PendingSoftwareJob.has_value(); });
            if (m_SoftwareWorkerStopRequested) {
                return;
            }
            job = std::move(*m_PendingSoftwareJob);
            m_PendingSoftwareJob.reset();
        }

        p_SWRenderer_->BeforeFrame(job.clearColor);
        p_SWRenderer_->SetFrameConfig(job.configSnapshot);
        p_SWRenderer_->SetMaterialState(job.materialState);
        p_SWRenderer_->SetFallbackTexture(job.fallbackTexture ? &*job.fallbackTexture : nullptr);
        p_SWRenderer_->SetSceneLights(job.lights);
        if (job.camera) {
            p_SWRenderer_->SetActiveCamera(*job.camera);
        } else {
            p_SWRenderer_->SetFallbackTexture(nullptr);
            continue;
        }
        if (job.configSnapshot.environment.showSkybox) {
            p_SWRenderer_->DrawSkybox();
        }
        if (job.scene) {
            auto& models = job.scene->m_Models;
            for (int modelIx : job.renderQueue) {
                {
                    std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
                    if (m_SoftwareWorkerStopRequested) {
                        return;
                    }
                }
                if (modelIx < 0 || static_cast<size_t>(modelIx) >= models.size()) {
                    continue;
                }
                const Model* model = &models[modelIx];
                if (!model->GetMeshes().empty()) {
                    p_SWRenderer_->DrawTriangularMesh(model);
                }
            }
        }
        p_SWRenderer_->EndFrame();
        if (job.configSnapshot.environment.showGrid) {
            p_SWRenderer_->DrawGridGizmo();
        }
        const auto& buffer = p_SWRenderer_->GetFrameBuffer();
        SoftwareCompletedFrame finishedFrame{};
        finishedFrame.width = buffer.width;
        finishedFrame.height = buffer.height;
        finishedFrame.jobId = job.jobId;
        finishedFrame.pixels.resize(buffer.GetCount());
        if (!finishedFrame.pixels.empty()) {
            std::memcpy(finishedFrame.pixels.data(), buffer.data, finishedFrame.pixels.size() * sizeof(Pixel));
        }

        {
            std::lock_guard<std::mutex> lock(m_SoftwareWorkerMutex);
            if (m_CompletedSoftwareFrames.size() >= kMaxBufferedSoftwareFrames) {
                m_CompletedSoftwareFrames.pop_front();
                p_stats->swFramesDroppedReady.fetch_add(1, std::memory_order_relaxed);
            }
            m_CompletedSoftwareFrames.push_back(std::move(finishedFrame));
        }
        p_SWRenderer_->SetFallbackTexture(nullptr);
        p_stats->swJobsCompleted.fetch_add(1, std::memory_order_relaxed);
    }
#endif
}

GLuint RenderSystem::RenderSoftwareSync(const std::shared_ptr<Scene>& scene,
                                        const Camera& camera,
                                        const std::vector<int>& renderQueue) {
    const Config configSnapshot = *Engine::Get().GetConfig();
    const SoftwareMaterialState materialState = CaptureSoftwareMaterialState();
    const std::optional<Texture> fallbackTexture = CaptureSoftwareFallbackTexture();
    p_SWRenderer_->BeforeFrame(m_SoftwareClearColor);
    p_SWRenderer_->SetFrameConfig(configSnapshot);
    p_SWRenderer_->SetMaterialState(materialState);
    p_SWRenderer_->SetFallbackTexture(fallbackTexture ? &*fallbackTexture : nullptr);
    p_SWRenderer_->SetSceneLights(scene ? scene->BuildLightSnapshots() : std::vector<LightSnapshot>{});
    p_SWRenderer_->SetActiveCamera(camera);
    if (configSnapshot.environment.showSkybox) {
        p_SWRenderer_->DrawSkybox();
    }
    if (scene) {
        auto& models = scene->m_Models;
        for (int modelIx : renderQueue) {
            if (modelIx < 0 || static_cast<size_t>(modelIx) >= models.size()) {
                continue;
            }
            const Model* model = &models[modelIx];
            if (!model->GetMeshes().empty()) {
                p_SWRenderer_->DrawTriangularMesh(model);
            }
        }
    }
    p_SWRenderer_->EndFrame();
    if (configSnapshot.environment.showGrid) {
        p_SWRenderer_->DrawGridGizmo();
    }
    const auto& buffer = p_SWRenderer_->GetFrameBuffer();
    m_SWFramePresenter.Upload(buffer);
    p_SWRenderer_->SetFallbackTexture(nullptr);
    return m_SWFramePresenter.GetTextureHandle();
}
} // namespace RetroRenderer
