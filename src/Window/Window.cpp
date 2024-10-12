#include "Window.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
    bool Window::Init()
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            LOGE("Failed to initialize SDL: %s", SDL_GetError());
            return false;
        }
        LOGI("SDL initialized successfully");
        return true;
    }

    void Window::Destroy()
    {
    }
}