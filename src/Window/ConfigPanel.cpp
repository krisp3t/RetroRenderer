/***
 * Wrapper around ImGui to create a configuration panel for the renderer.
 */

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>
#include "ConfigPanel.h"

namespace RetroRenderer
{
    ConfigPanel::ConfigPanel(SDL_Window* window, SDL_Renderer* renderer)
    {
        Init(window, renderer);
    }

    ConfigPanel::~ConfigPanel()
    {
        Destroy();
    }

    bool ConfigPanel::Init(SDL_Window* window, SDL_Renderer* renderer)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        return true;
    }

    void ConfigPanel::DisplayGUI()
    {
        bool show = true;
        ImGui::ShowDemoWindow(&show);
        ImGui::ShowDebugLogWindow(&show);
        ImGui::ShowMetricsWindow(&show);

        DisplayMetricsOverlay();
    }

    void ConfigPanel::DisplayMetricsOverlay()
    {
        static bool isOpen = true;
        if (!isOpen) return;

        static int location = 1;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        constexpr float kPadding = 10.0f;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - kPadding) : (work_pos.x + kPadding);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - kPadding) : (work_pos.y + kPadding);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        windowFlags |= ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("Example: Simple overlay", &isOpen, windowFlags))
        {
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Top-left",     NULL, location == 0)) location = 0;
                if (ImGui::MenuItem("Top-right",    NULL, location == 1)) location = 1;
                if (ImGui::MenuItem("Bottom-left",  NULL, location == 2)) location = 2;
                if (ImGui::MenuItem("Bottom-right", NULL, location == 3)) location = 3;
                if (ImGui::MenuItem("Close")) isOpen = false;
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    void ConfigPanel::BeforeFrame(SDL_Renderer* renderer) {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        DisplayGUI();

        ImGui::Render();
        auto const io = ImGui::GetIO();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    }

    void ConfigPanel::OnDraw()
    {
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    }

    void ConfigPanel::Destroy()
    {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }


}