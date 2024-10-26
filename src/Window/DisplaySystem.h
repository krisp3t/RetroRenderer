#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include "ConfigPanel.h"
#include "../Renderer/Buffer.h"

namespace RetroRenderer
{

class DisplaySystem
{
public:
    DisplaySystem() = default;
    ~DisplaySystem() = default;

    bool Init();
    void Destroy();

    static constexpr SDL_WindowFlags kWindowFlags =
            static_cast<const SDL_WindowFlags>(
                    SDL_WINDOW_RESIZABLE |
                    SDL_WINDOW_ALLOW_HIGHDPI);
    static constexpr SDL_RendererFlags kRendererFlags =
            static_cast<const SDL_RendererFlags>(
                    SDL_RENDERER_PRESENTVSYNC
            );
    static constexpr char kWindowTitle[] = "RetroRenderer";
    std::shared_ptr<Config> p_Config = nullptr;

    void Clear();
    void DrawConfigPanel();
    void SwapBuffers();
    void BeforeFrame();
    void DrawFrame();
    void DrawFrame(const Buffer<Uint32> &buffer);
    int GetWidth() const;
    int GetHeight() const;

private:
    SDL_Window* m_Window = nullptr;
    SDL_Renderer* m_SDLRenderer = nullptr;
    SDL_Texture* m_ScreenTexture = nullptr;
    int m_Width = 1280;
    int m_Height = 720;
    std::unique_ptr<ConfigPanel> m_ConfigPanel = nullptr;
    void OnResize(int width, int height);

};

}