#pragma once

#include "../Base/Color.h"
#include "../Base/Stats.h"
#include "../Renderer/Buffer.h"
#include "../Renderer/RenderOutput.h"
#include "../Scene/Camera.h"
#include "ConfigPanel.h"
#include <SDL.h>
#include <memory>

namespace RetroRenderer {
class DisplaySystem {
  public:
#ifdef __ANDROID__
    static constexpr SDL_WindowFlags kWindowFlags = static_cast<const SDL_WindowFlags>(
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#elif defined(__EMSCRIPTEN__)
    static constexpr SDL_WindowFlags kWindowFlags =
        static_cast<const SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
#else
    static constexpr SDL_WindowFlags kWindowFlags =
        static_cast<const SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif
    static constexpr char kWindowTitle[] = "RetroRenderer";

    DisplaySystem() = default;

    ~DisplaySystem() = default;

    SDL_Window* GetWindow() const;

    bool Init(const std::shared_ptr<Config>& config, const std::shared_ptr<Stats>& stats);

    void Destroy();

    void SwapBuffers();

    void BeforeFrame();

    void DrawFrame();

    void DrawFrame(const RenderOutput& output);

    void ResetGlContext();

  private:
    SDL_Window* m_window_ = nullptr;
    SDL_GLContext m_glContext_ = nullptr;
    std::unique_ptr<ConfigPanel> m_configPanel_ = nullptr;
};

} // namespace RetroRenderer
