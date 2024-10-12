#include "DisplayManager.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
    bool DisplayManager::Init()
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

        m_ConfigPanel = std::make_unique<ConfigPanel>(m_Window, m_SDLRenderer);
        return true;
    }

    void DisplayManager::Clear()
    {
        SDL_SetRenderDrawColor(m_SDLRenderer, 0xFF, 0, 0xFF, 0xFF);
        SDL_RenderClear(m_SDLRenderer);
    }

    void DisplayManager::DrawConfigPanel()
    {
        m_ConfigPanel.get()->OnDraw(m_SDLRenderer);
    }

    void DisplayManager::SwapBuffers()
    {
        SDL_RenderPresent(m_SDLRenderer);
    }

    void DisplayManager::Destroy()
    {
        SDL_DestroyRenderer(m_SDLRenderer);
        SDL_DestroyWindow(m_Window);
        SDL_Quit();
    }


}