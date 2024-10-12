#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include "ConfigPanel.h"

namespace RetroRenderer
{

class DisplayManager
{
public:
    DisplayManager() = default;
    ~DisplayManager() = default;

    bool Init();
    void Destroy();

    static constexpr SDL_WindowFlags kWindowFlags =
            static_cast<const SDL_WindowFlags>(
                    SDL_WINDOW_RESIZABLE |
                    SDL_WINDOW_ALLOW_HIGHDPI);
    static constexpr SDL_RendererFlags kRendererFlags =
            static_cast<const SDL_RendererFlags>(
                    SDL_RENDERER_PRESENTVSYNC |
                    SDL_RENDERER_SOFTWARE);
    static constexpr char kWindowTitle[] = "RetroRenderer";

private:
    SDL_Window* m_Window;
    SDL_Renderer* m_SDLRenderer;
    int m_Width = 1280;
    int m_Height = 720;
    std::unique_ptr<ConfigPanel> m_ConfigPanel;

    void Resize(int width, int height);
};

}
