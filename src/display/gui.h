#pragma once

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "../settings.h"
#include "utility.h"
#include <SDL.h>
#include "../../lib/ImGuiFileDialog/ImGuiFileDialog.h"
#include "../model.h"
#include "../tgaimage.h"

namespace MiniRenderer {
    enum GUIState {
        Error,
        None,
        LoadModel,
        SaveScreenshot
    };

    class GUI
    {
    public:
        GUI() = default;
        ~GUI();
        bool initialize_gui(SDL_Window* window, SDL_Renderer* renderer);
        void process_input(SDL_Event& event);
        GUIState update(Settings& s, const Model& m);
        void render();
        void destroy_gui();
    private:
        SDL_Window* mWindow;
        SDL_Renderer* mRenderer;
        ImGuiIO* mIo;
    };
}