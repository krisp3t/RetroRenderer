#include <imgui_impl_sdl2.h>
#include "InputManager.h"

namespace RetroRenderer
{
    bool InputManager::Init(DisplayManager &displayManager)
    {
        return true;
    }

    void InputManager::HandleInput(bool &isRunning)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    isRunning = false;
                }
                break;
            }
        }
    }

    void InputManager::Update()
    {
    }

    void InputManager::Destroy()
    {
    }
}