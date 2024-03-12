#include "gui.h"

namespace MiniRenderer {
    bool GUI::initialize_gui(SDL_Window* window, SDL_Renderer* renderer) {
        this->window = window;
        this->renderer = renderer;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        this->io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        ImGui::StyleColorsDark();

        // Setup SDLRenderer2 binding
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        return true;
    }

    void GUI::destroy_gui() {
		ImGui_ImplSDLRenderer2_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

    GUI::~GUI() {
        GUI::destroy_gui();
    }

    void GUI::process_input(SDL_Event& event) {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }

    void GUI::update(Settings& s) {
        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow(&s.open_windows.show_renderer_window); // TODO: remove this


        // Rendering settings window
        if (s.open_windows.show_renderer_window) {

            ImGui::Begin("Rendering settings");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
            ImGui::End();
        }

        // Camera settings window
        if (s.open_windows.show_camera_window) {
            ImGui::Begin("Camera settings");
            ImGui::SliderFloat3("Position", &s.camera.position[0], -100.0f, 100.0f);
            ImGui::SliderFloat3("Rotation", &s.camera.rotation[0], -180.0f, 180.0f);
            // ImGui::Text("Camera rotation: (%.2f, %.2f, %.2f)", s->camera.rotation.x, s->camera.rotation.y, s->camera.rotation.z);
            ImGui::End();
        }
        ImGui::Render();
    }

    void GUI::render() {

		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
	}
}
