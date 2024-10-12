#include "Engine.h"

namespace RetroRenderer
{
    bool Engine::Init()
    {
        if (!mWindow.Init())
        {
            return false;
        }
        return true;
    }

    void Engine::Destroy()
    {
    }
}