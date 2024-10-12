#include "Engine.h"
#include "Base/Logger.h"

namespace RetroRenderer
{
    bool Engine::Init()
    {
        if (!m_DisplayManager.Init())
        {
            return false;
        }
        if (!m_RenderManager.Init(m_DisplayManager))
        {
            return false;
        }
        return true;
    }

    void Engine::Run()
    {
        bool isRunning = true;
        auto start = SDL_GetTicks();
        auto delta = 0;

        LOGI("Entered main loop");
        while (isRunning)
        {
            start = SDL_GetTicks();

            m_RenderManager.Render();

            delta = SDL_GetTicks() - start;
        }
    }

    void Engine::Destroy()
    {
    }
}