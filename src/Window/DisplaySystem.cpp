#include <imgui.h>
#include "DisplaySystem.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
    bool DisplaySystem::Init()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            LOGE("Unable to initialize SDL: %s\n", SDL_GetError());
            return false;
        }
        m_Window = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_Width, m_Height, kWindowFlags);
        if (m_Window == nullptr)
        {
            LOGE("Unable to create window: %s", SDL_GetError());
            return false;
        }
        m_SDLRenderer = SDL_CreateRenderer(m_Window, -1, kRendererFlags);
        if (m_SDLRenderer == nullptr)
        {
            LOGE("Unable to create renderer: %s", SDL_GetError());
            return false;
        }

        p_Config = std::make_shared<Config>();
        m_ConfigPanel = std::make_unique<ConfigPanel>(m_Window, m_SDLRenderer, p_Config);
        return true;
    }

    void DisplaySystem::BeforeFrame() {
        m_ConfigPanel.get()->BeforeFrame(m_SDLRenderer);
        ImU32 c = ImGui::ColorConvertFloat4ToU32(p_Config->renderer.clearColor);
        SDL_SetRenderDrawColor(m_SDLRenderer, c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, (c >> 24) & 0xFF);
        SDL_RenderClear(m_SDLRenderer);
    }

    void DisplaySystem::DrawFrame() {
        m_ConfigPanel.get()->OnDraw();
    }

    void DisplaySystem::SwapBuffers()
    {
        SDL_RenderPresent(m_SDLRenderer);
    }

    void DisplaySystem::Destroy()
    {
        SDL_DestroyRenderer(m_SDLRenderer);
        SDL_DestroyWindow(m_Window);
        SDL_Quit();
    }

}