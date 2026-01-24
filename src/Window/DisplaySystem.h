#pragma once

#include "../Base/Color.h"
#include "../Base/Stats.h"
#include "../Renderer/Buffer.h"
#include "../Scene/Camera.h"
#include "../include/kris_glheaders.h"
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

    bool Init();

    void Destroy();

    void SwapBuffers();

    void BeforeFrame();

    void DrawFrame();

    void DrawFrame(GLuint framebufferTexture);

    void ResetGlContext();

  private:
    SDL_Window* m_window_ = nullptr;
    SDL_GLContext m_glContext_ = nullptr;
    std::unique_ptr<ConfigPanel> m_configPanel_ = nullptr;
};

} // namespace RetroRenderer
