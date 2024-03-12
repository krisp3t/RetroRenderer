#pragma once

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "../settings.h"
#include <SDL.h>


namespace MiniRenderer {
    class GUI
    {
    public:
        GUI() = default;
        ~GUI();
        bool initialize_gui(SDL_Window* window, SDL_Renderer* renderer);
        void process_input(SDL_Event& event);
        void update(Settings& settings);
        void render();
        void destroy_gui();
    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        ImGuiIO* io;
    };
}