#pragma once
#include <SDL2/SDL.h>

namespace RetroRenderer
{

class Window
{
public:
    Window() = default;
    ~Window() = default;

    bool Init();
    void Destroy();
private:
    SDL_Window* mWindow;
};

}


