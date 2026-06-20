#include "InlineRenderExecutor.h"
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#include "GLES/GLESRenderer.h"
#else
#include "OpenGL/GLRenderer.h"
#endif
#include <KrisLogger/Logger.h>
#include <cassert>
#include <chrono>

namespace RetroRenderer {
namespace {
using TimingClock = std::chrono::steady_clock;

uint64_t ElapsedNanoseconds(TimingClock::time_point start) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(TimingClock::now() - start).count());
}
} // namespace

bool InlineRenderExecutor::Init(SDL_Window* window,
                                SDL_GLContext context,
                                const std::shared_ptr<Config>& config,
                                const std::shared_ptr<Stats>& stats,
                                const UiFontAtlas& fontAtlas) {
    m_Window = window;
    m_Context = context;
    p_Stats = stats;
    m_OwnerThread = std::this_thread::get_id();
    AssertOwnerThread();
    SDL_GL_MakeCurrent(m_Window, m_Context);

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        LOGE("Error initializing GLAD in render executor");
        return false;
    }
#endif

    const glm::ivec2 initialSize = config->renderer.resolution;
    if (!m_OutputPresenter.Init(initialSize.x, initialSize.y, config->renderer.nearestNeighborPresentation) ||
        !m_PreviewPresenter.Init(1, 1, false) ||
        !m_FontPresenter.Init(fontAtlas.width, fontAtlas.height, false) ||
        !m_FontPresenter.UploadPixels(fontAtlas.pixels.data(),
                                      static_cast<size_t>(fontAtlas.width),
                                      static_cast<size_t>(fontAtlas.height)) ||
        !m_UiRenderer.Init()) {
        LOGE("Failed to initialize render executor presentation resources");
        Destroy();
        return false;
    }

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    m_GLRenderer = std::make_unique<GLESRenderer>();
#else
    m_GLRenderer = std::make_unique<GLRenderer>();
#endif
    if (!m_GLRenderer->Init(m_OutputPresenter.GetTextureHandle(), initialSize.x, initialSize.y)) {
        LOGE("Failed to initialize hardware renderer in render executor");
        Destroy();
        return false;
    }

    m_OutputSize = initialSize;
    m_NearestNeighbor = config->renderer.nearestNeighborPresentation;
    m_Vsync = config->window.enableVsync;
#ifndef __EMSCRIPTEN__
    SDL_GL_SetSwapInterval(m_Vsync ? 1 : 0);
#endif
    m_Initialized = true;
    return true;
}

void InlineRenderExecutor::AssertOwnerThread() const {
    assert(m_OwnerThread == std::this_thread::get_id() && "GL render executor called from a non-owner thread");
}

bool InlineRenderExecutor::ApplyOutputConfiguration(const RenderPacket& packet) {
    const glm::ivec2 requestedSize = packet.configSnapshot.renderer.resolution;
    const bool nearest = packet.configSnapshot.renderer.nearestNeighborPresentation;
    if (requestedSize == m_OutputSize && nearest == m_NearestNeighbor) {
        return true;
    }
    if (!m_OutputPresenter.Resize(requestedSize.x, requestedSize.y, nearest)) {
        return false;
    }
    m_GLRenderer->Resize(m_OutputPresenter.GetTextureHandle(), requestedSize.x, requestedSize.y);
    m_OutputSize = requestedSize;
    m_NearestNeighbor = nearest;
    return true;
}

void InlineRenderExecutor::ApplyUiTextureSnapshots(const std::vector<UiTextureSnapshot>& snapshots) {
    for (const UiTextureSnapshot& snapshot : snapshots) {
        if (snapshot.handle.value != PresentationTexture::MaterialPreview.value || !snapshot.texture ||
            !snapshot.texture->HasCpuPixels()) {
            continue;
        }
        const Texture& texture = *snapshot.texture;
        m_PreviewPresenter.Resize(texture.GetWidth(), texture.GetHeight(), snapshot.nearestNeighbor);
        const auto& pixels = texture.GetPixels();
        m_PreviewPresenter.UploadPixels(pixels.data(),
                                        static_cast<size_t>(texture.GetWidth()),
                                        static_cast<size_t>(texture.GetHeight()));
    }
}

GLuint InlineRenderExecutor::ResolveUiTexture(TextureHandle handle) const {
    if (handle.value == PresentationTexture::Output.value) {
        return static_cast<GLuint>(m_OutputPresenter.GetTextureHandle().value);
    }
    if (handle.value == PresentationTexture::MaterialPreview.value) {
        return static_cast<GLuint>(m_PreviewPresenter.GetTextureHandle().value);
    }
    if (handle.value == PresentationTexture::FontAtlas.value) {
        return static_cast<GLuint>(m_FontPresenter.GetTextureHandle().value);
    }
    return 0;
}

void InlineRenderExecutor::Execute(FrameSubmission&& submission) {
    AssertOwnerThread();
    if (!m_Initialized || !submission.renderPacket) {
        return;
    }
    SDL_GL_MakeCurrent(m_Window, m_Context);
    const RenderPacket& packet = *submission.renderPacket;
    if (!ApplyOutputConfiguration(packet)) {
        return;
    }

    if (submission.enableVsync != m_Vsync) {
        m_Vsync = submission.enableVsync;
#ifndef __EMSCRIPTEN__
        SDL_GL_SetSwapInterval(m_Vsync ? 1 : 0);
#endif
    }

    if (packet.hasScene && packet.configSnapshot.renderer.selectedRenderer == Config::RendererType::GL) {
        const auto glRenderStart = TimingClock::now();
        m_GLRenderer->RenderFrame(packet);
        p_Stats->lastGlRenderNs.store(ElapsedNanoseconds(glRenderStart), std::memory_order_relaxed);
    } else if (submission.softwareFrame && !submission.softwareFrame->pixels.empty()) {
        const CpuFrame& frame = *submission.softwareFrame;
        const auto uploadStart = TimingClock::now();
        m_OutputPresenter.UploadPixels(frame.pixels.data(), frame.width, frame.height);
        p_Stats->lastCpuOutputUploadNs.store(ElapsedNanoseconds(uploadStart), std::memory_order_relaxed);
    }

    ApplyUiTextureSnapshots(submission.uiTextures);
    const auto uiRenderStart = TimingClock::now();
    m_UiRenderer.Render(submission.ui, [this](TextureHandle handle) { return ResolveUiTexture(handle); });
    p_Stats->lastImGuiRenderNs.store(ElapsedNanoseconds(uiRenderStart), std::memory_order_relaxed);
    const auto swapStart = TimingClock::now();
    SDL_GL_SwapWindow(m_Window);
    p_Stats->lastSwapBuffersNs.store(ElapsedNanoseconds(swapStart), std::memory_order_relaxed);
}

void InlineRenderExecutor::Destroy() {
    if (m_OwnerThread != std::thread::id{}) {
        AssertOwnerThread();
    }
    if (m_Window && m_Context) {
        SDL_GL_MakeCurrent(m_Window, m_Context);
    }
    if (m_GLRenderer) {
        m_GLRenderer->Destroy();
        m_GLRenderer.reset();
    }
    m_UiRenderer.Destroy();
    m_FontPresenter.Destroy();
    m_PreviewPresenter.Destroy();
    m_OutputPresenter.Destroy();
    m_Initialized = false;
}

} // namespace RetroRenderer
