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
        if (!m_InputManager.Init(m_DisplayManager.p_Config))
        {
            return false;
        }
        LOGD("p_Config ref count: %d", m_DisplayManager.p_Config.use_count());
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

            m_InputManager.HandleInput(isRunning);
            m_RenderManager.Render();

            delta = SDL_GetTicks() - start;
        }
    }

    void Engine::Destroy()
    {
    }
}