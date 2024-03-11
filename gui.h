#pragma once

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "settings.h"
#include <SDL.h>

namespace MiniRenderer {
    class GUI
    {
    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        ImGuiIO* io;
        ImVec4 clear_color;
    public:
        GUI(SDL_Window* window, SDL_Renderer* renderer);
        ~GUI();
        void Render(int* red);
        void ProcessEvent(SDL_Event& event);
        void Render(Settings* settings);
    };
}