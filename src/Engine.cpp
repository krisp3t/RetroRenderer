#include "Engine.h"
#include "Base/MemoryProfiler.h"
#include "Renderer/InlineRenderExecutor.h"
#include <KrisLogger/Logger.h>
#include <algorithm>
#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>

namespace {
RetroRenderer::Engine* g_Engine = nullptr;

extern "C" void MainLoopWrapper() {
    if (g_Engine) {
        g_Engine->ProcessFrame();
    }
}
} // namespace
#endif

namespace RetroRenderer {
namespace {
using TimingClock = std::chrono::steady_clock;

uint64_t ElapsedNanoseconds(TimingClock::time_point start) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(TimingClock::now() - start).count());
}
} // namespace

bool Engine::Init() {
#ifndef __EMSCRIPTEN__
    LOGD("Starting RetroRenderer in directory: %s", SDL_GetBasePath());
#endif
    if (!m_DisplaySystem.Init(p_config_, p_stats_)) {
        return false;
    }
    p_RenderExecutor = std::make_unique<InlineRenderExecutor>();
    if (!p_RenderExecutor->Init(m_DisplaySystem.GetWindow(),
                                m_DisplaySystem.GetGlContext(),
                                p_config_,
                                p_stats_,
                                m_DisplaySystem.GetFontAtlas())) {
        return false;
    }
    if (!m_InputSystem.Init(p_config_, [this](std::unique_ptr<Event> event) { EnqueueEvent(std::move(event)); })) {
        return false;
    }
    p_MaterialManager = std::make_unique<MaterialManager>();
    p_RenderSystem = std::make_unique<RenderSystem>(p_config_, p_stats_, *p_MaterialManager);
    if (!p_RenderSystem->Init()) {
        return false;
    }
    p_SceneManager = std::make_unique<SceneManager>();
    p_SceneManager->BindDependencies(p_config_, *p_RenderSystem);
    LOGD("p_Config_ ref count: %d", p_config_.use_count());
    p_MaterialManager->BindRenderServices(*p_RenderSystem);
    p_MaterialManager->BindSceneAccessor([this]() {
        return p_SceneManager ? p_SceneManager->GetScene() : nullptr;
    });
    if (!p_MaterialManager->Init()) {
        return false;
    }
    m_DisplaySystem.BindEditorContext(EditorContext{
        .config = p_config_,
        .stats = p_stats_,
        .sceneManager = p_SceneManager.get(),
        .materialManager = p_MaterialManager.get(),
        .dispatchImmediate = [this](const Event& event) { DispatchImmediate(event); },
        .enqueueEvent = [this](std::unique_ptr<Event> event) { EnqueueEvent(std::move(event)); },
        .selectedModelIndex = std::nullopt,
    });

    // Default scene (optional)
    // p_SceneManager->LoadScene("frog/frog.obj");

    // p_SceneManager->LoadScene("tests-visual/basic-tests/03-3d-cube/model-quad.obj");
    return true;
}

void Engine::Run() {
    m_LastFrameTicks = SDL_GetTicks();

    LOGD("Entered main loop");
#ifdef __EMSCRIPTEN__
    g_Engine = this;
    emscripten_set_main_loop(MainLoopWrapper, 0, 1);
#else
    while (m_Running) {
        ProcessFrame();
    }
#endif
}

void Engine::ProcessFrame() {
    const auto frameStart = TimingClock::now();
    const Uint32 now = SDL_GetTicks();
    const Uint32 rawDelta = now - m_LastFrameTicks;
    m_LastFrameTicks = now;
    const Uint32 delta = std::min<Uint32>(rawDelta, 50);

    const auto mainUpdateStart = TimingClock::now();
    ProcessEventQueue();

    auto inputActions = m_InputSystem.HandleInput();
    if (inputActions & static_cast<InputActionMask>(InputAction::QUIT)) {
        m_Running = false;
        p_stats_->lastFrameTotalNs.store(ElapsedNanoseconds(frameStart), std::memory_order_relaxed);
        return;
    }
    // TODO: handle camera switching

    p_SceneManager->ProcessInput(inputActions, delta);
    p_SceneManager->Update(delta, p_config_->renderer.resolution);
    p_SceneManager->NewFrame();

    const ProcessMemorySnapshot memorySnapshot = MemoryProfiler::SampleProcessMemory();
    if (memorySnapshot.supported) {
        p_stats_->UpdateProcessMemory(memorySnapshot.residentBytes, memorySnapshot.peakResidentBytes);
    }
    p_stats_->lastMainUpdateNs.store(ElapsedNanoseconds(mainUpdateStart), std::memory_order_relaxed);

    auto scene = p_SceneManager->GetScene();
    auto camera = p_SceneManager->GetCamera();
    const bool hasScene = scene != nullptr && camera != nullptr;

    const auto beforeFrameStart = TimingClock::now();
    m_DisplaySystem.BeforeFrame();
    p_stats_->lastDisplayBeforeFrameNs.store(ElapsedNanoseconds(beforeFrameStart), std::memory_order_relaxed);

    p_RenderSystem->BeforeFrame(p_config_->renderer.clearColor);
    const bool outputAvailable =
        hasScene && (p_config_->renderer.selectedRenderer == Config::RendererType::GL ||
                     p_RenderSystem->PollSoftwareFrame());
    const auto drawStart = TimingClock::now();
    if (hasScene) {
        const RenderOutputOrigin origin = p_config_->renderer.selectedRenderer == Config::RendererType::GL
                                              ? RenderOutputOrigin::BottomLeft
                                              : RenderOutputOrigin::TopLeft;
        m_DisplaySystem.DrawFrame(outputAvailable, origin);
    } else {
        p_stats_->Reset();
        m_DisplaySystem.DrawFrame();
    }
    p_stats_->lastDisplayDrawNs.store(ElapsedNanoseconds(drawStart), std::memory_order_relaxed);

    const auto packetStart = TimingClock::now();
    const std::shared_ptr<const RenderPacket> packet = p_RenderSystem->BuildRenderPacket(scene, camera);
    p_stats_->lastRenderPacketBuildNs.store(ElapsedNanoseconds(packetStart), std::memory_order_relaxed);

    std::shared_ptr<const CpuFrame> softwareFrame;
    if (hasScene) {
        softwareFrame = p_RenderSystem->PrepareFrame(packet);
    } else {
        p_stats_->lastRenderSystemNs.store(0, std::memory_order_relaxed);
        p_stats_->lastGlRenderNs.store(0, std::memory_order_relaxed);
        p_stats_->lastSoftwarePacketCopyNs.store(0, std::memory_order_relaxed);
    }

    FrameSubmission submission{};
    submission.frameId = ++m_NextFrameId;
    submission.renderPacket = packet;
    submission.softwareFrame = std::move(softwareFrame);
    submission.uiTextures = m_DisplaySystem.TakeUiTextureSnapshots();
    submission.ui = m_DisplaySystem.TakeUiRenderPacket();
    submission.enableVsync = p_config_->window.enableVsync;
    p_RenderExecutor->Execute(std::move(submission));
    p_stats_->lastFrameTotalNs.store(ElapsedNanoseconds(frameStart), std::memory_order_relaxed);
}

void Engine::Destroy() {
    if (p_RenderSystem) {
        p_RenderSystem->Destroy();
        p_RenderSystem.reset();
    }
    if (p_RenderExecutor) {
        p_RenderExecutor->Destroy();
        p_RenderExecutor.reset();
    }
    m_DisplaySystem.Destroy();
}

void Engine::EnqueueEvent(std::unique_ptr<Event> event) {
    LOGD("Enqueuing event (type: %s)", EventTypeToString(event->type));
    std::lock_guard<std::mutex> lock(m_EventQueueMutex);
    m_EventQueue.push(std::move(event));
}

void Engine::ProcessEventQueue() {
    std::queue<std::unique_ptr<Event>> tempQueue;
    {
        // Move all events out of the main queue in one lock
        std::lock_guard<std::mutex> lock(m_EventQueueMutex);
        std::swap(m_EventQueue, tempQueue);
    }
    // Process outside of lock to avoid holding it during event handling
    while (!tempQueue.empty()) {
        auto& event = *tempQueue.front();
        Dispatch(event);
        tempQueue.pop();
    }
}

/**
 * Handle synchronous event (immediately, without queue).
 * Used for events that need to be handled immediately, like window resizing and scene reloading.
 */
void Engine::DispatchImmediate(const Event& event) {
    // map EventType enum to string
    // https://stackoverflow.com/questions/147267/easy-way-to-use-variables-of-enum-types-as-string-in-c
    LOGD("Dispatching immediate event (type: %s)", EventTypeToString(event.type));
    Dispatch(event);
}

void Engine::Dispatch(const Event& event) {
    LOGI("Handling event (type: %s)", EventTypeToString(event.type));
    switch (event.type) {
    case EventType::Texture_Load: {
        const TextureLoadEvent& e = static_cast<const TextureLoadEvent&>(event);
        if (e.loadFromMemory) {
            p_MaterialManager->LoadTexture(e.textureDataBuffer.data(), e.textureDataSize);
        } else {
            LOGE("Not implemented yet");
        }
        // p_RenderSystem->OnLoadScene(e); // TODO: send to all subscribers
        break;
    }
    case EventType::Scene_Load: {
        const SceneLoadEvent& e = static_cast<const SceneLoadEvent&>(event);
        if (!e.loadFromMemory) {
            LOGD("Attempting to load scene from path: %s", e.scenePath.c_str());
            p_SceneManager->LoadScene(e.scenePath, e.appendToCurrentScene);
        } else {
            p_SceneManager->LoadScene(e.sceneDataBuffer.data(), e.sceneDataSize, e.appendToCurrentScene);
        }
        p_RenderSystem->OnLoadScene(e); // TODO: send to all subscribers
        break;
    }
    case EventType::Scene_Reset: {
        p_SceneManager->ResetScene();
        p_RenderSystem->OnResetScene();
        break;
    }
    case EventType::Output_Image_Resize: {
        const OutputImageResizeEvent& e = static_cast<const OutputImageResizeEvent&>(event);
        if (e.resolution.x <= 0 || e.resolution.y <= 0) {
            break;
        }
        p_RenderSystem->Resize(e.resolution);
        break;
    }
    case EventType::Window_Resize: {
        const WindowResizeEvent& e = static_cast<const WindowResizeEvent&>(event);
        LOGD("Window resized to %d x %d", e.resolution.x, e.resolution.y);
        if (e.resolution.x <= 0 || e.resolution.y <= 0) {
            break;
        }
        auto& p_config = GetConfig();
        p_config->window.size.x = e.resolution.x;
        p_config->window.size.y = e.resolution.y;
        break;
    }
    default:
        LOGW("Unknown event type");
    }
}

const std::shared_ptr<Config> Engine::GetConfig() const {
    return p_config_;
}

const std::shared_ptr<Stats> Engine::GetStats() const {
    return p_stats_;
}

Camera* Engine::GetCamera() const {
    return p_SceneManager->GetCamera();
}

MaterialManager& Engine::GetMaterialManager() const {
    return *p_MaterialManager;
}

RenderSystem& Engine::GetRenderSystem() const {
    return *p_RenderSystem;
}

SceneManager& Engine::GetSceneManager() const {
    return *p_SceneManager;
}
} // namespace RetroRenderer
