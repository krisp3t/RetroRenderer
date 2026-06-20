#pragma once

#include "../Base/Color.h"
#include "../Base/Stats.h"
#include "../Renderer/Buffer.h"
#include "../Renderer/RenderOutput.h"
#include "../Scene/Camera.h"
#include "ConfigPanel.h"
#include "EditorContext.h"
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
    SDL_GLContext GetGlContext() const;
    [[nodiscard]] const UiFontAtlas& GetFontAtlas() const;
    [[nodiscard]] UiRenderPacket TakeUiRenderPacket();
    [[nodiscard]] std::vector<UiTextureSnapshot> TakeUiTextureSnapshots();

    bool Init(const std::shared_ptr<Config>& config, const std::shared_ptr<Stats>& stats);
    void BindEditorContext(EditorContext editorContext);

    void Destroy();

    void BeforeFrame();

    void DrawFrame();

    void DrawFrame(bool outputAvailable, RenderOutputOrigin origin);

  private:
    SDL_Window* m_window_ = nullptr;
    SDL_GLContext m_glContext_ = nullptr;
    std::unique_ptr<ConfigPanel> m_configPanel_ = nullptr;
};

} // namespace RetroRenderer
