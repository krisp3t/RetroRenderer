/***
 * Wrapper around ImGui to create a configuration panel for the renderer.
 */

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>
#include "../../lib/ImGuiFileDialog/ImGuiFileDialog.h"
#include "ConfigPanel.h"
#include "../Base/Logger.h"

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

        DisplayMainMenu();
        DisplayMetricsOverlay();
        DisplayRendererSettings();
    }

    void ConfigPanel::DisplayMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            // Menu items
            // ----------------
            if (ImGui::MenuItem("Open model"))
            {
                IGFD::FileDialogConfig config;
                ImGuiFileDialog::Instance()->OpenDialog("OpenModelFile", "Choose model (.obj)", ".obj", config);
            }

            if (ImGui::MenuItem("Reset"))
            {
                LOGD("Clearing scene");
            }

            if (ImGui::MenuItem("About"))
            {
                ImGui::OpenPopup("About");
            }

            // Dialog windows
            // ----------------
            // Open Model File dialog
            if (ImGuiFileDialog::Instance()->Display("OpenModelFile")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    LOGD("Selected model file: %s", ImGuiFileDialog::Instance()->GetFilePathName().c_str());
                    /*
                    s.filename = ImGuiFileDialog::Instance()->GetFilePathName();
                    s.filepath = ImGuiFileDialog::Instance()->GetCurrentPath();
                    state = GUIState::LoadModel;
                    */
                }
                ImGuiFileDialog::Instance()->Close();
            }

            // About dialog
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Appearing);
            if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
            {
                // TODO: Add version number
                ImGui::TextWrapped("RetroRenderer v%d.%d", 0, 1);
                ImGui::TextWrapped("Software renderer with fixed-function pipeline for educational purposes.");
                ImGui::Separator();
                ImGui::TextWrapped("Built by krisp3t under the MIT License. Enjoy!");

                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 120.0f) * 0.5f);
                ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 30.0f);
                if (ImGui::Button("Close", ImVec2(120.0f, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void ConfigPanel::DisplayRendererSettings()
    {
        ImGui::Begin("Renderer settings");
        if (ImGui::Button("Screenshot"))
        {
            // TODO: implement screenshot
            LOGD("Taking screenshot");
        }
        ImGui::SeparatorText("Scene");

        // Background color
        static ImVec4 clearColor = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
        ImGui::ColorEdit4("Color", (float*)&clearColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        ImGui::Text("Background color:");

        ImGui::End();
    }

    void ConfigPanel::DisplayMetricsOverlay()
    {
        static bool isOpen = true;
        if (!isOpen) return;

        static int location = 1;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        constexpr float kPadding = 10.0f;

        static float frameTimes[100] = { 0 };  // Buffer for frame times
        static int frameIndex = 0;
        frameTimes[frameIndex] = 1000.0f / io.Framerate;  // Store current frame time in milliseconds
        frameIndex = (frameIndex + 1) % 100;  // Wrap index around when it reaches the buffer size


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
        if (ImGui::Begin("Metrics", &isOpen, windowFlags))
        {
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("%d verts, %d tris", 0, 0);
            ImGui::PlotLines("", frameTimes, IM_ARRAYSIZE(frameTimes), frameIndex, nullptr, 0.0f, 50.0f, ImVec2(0, 80));
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Top-left",nullptr, location == 0)) location = 0;
                if (ImGui::MenuItem("Top-right", nullptr, location == 1)) location = 1;
                if (ImGui::MenuItem("Bottom-left",nullptr, location == 2)) location = 2;
                if (ImGui::MenuItem("Bottom-right",nullptr, location == 3)) location = 3;
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