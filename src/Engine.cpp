#include "Engine.h"

namespace RetroRenderer
{
    bool Engine::Init()
    {
        if (!m_DisplayManager.Init())
        {
            return false;
        }
        return true;
    }

    void Engine::Destroy()
    {
    }
}