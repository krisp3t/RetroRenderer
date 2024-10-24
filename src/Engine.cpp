#include "Engine.h"
#include "Base/Logger.h"

namespace RetroRenderer
{
    bool Engine::Init()
    {
        if (!m_DisplaySystem.Init())
        {
            return false;
        }
        if (!m_RenderSystem.Init(m_DisplaySystem))
        {
            return false;
        }
        if (!m_InputSystem.Init(m_DisplaySystem.p_Config))
        {
            return false;
        }
        LOGD("p_Config ref count: %d", m_DisplaySystem.p_Config.use_count());
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

            m_InputSystem.HandleInput(isRunning);
            m_RenderSystem.Render();

            delta = SDL_GetTicks() - start;
        }
    }

    void Engine::Destroy()
    {
    }
}