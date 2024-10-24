/***
 * Wrapper around ImGui to create a configuration panel for the renderer.
 */
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

#include "../../lib/ImGuiFileDialog/ImGuiFileDialog.h"
#include "ConfigPanel.h"
#include "../Base/Logger.h"
#include "../Base/InputActions.h"

namespace RetroRenderer
{
    ConfigPanel::ConfigPanel(SDL_Window *window, SDL_Renderer *renderer, std::shared_ptr<Config> config)
    {
        Init(window, renderer, config);
    }

    ConfigPanel::~ConfigPanel()
    {
        Destroy();
    }

    bool ConfigPanel::Init(SDL_Window* window, SDL_Renderer* renderer, std::shared_ptr<Config> config)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        p_Config = std::move(config);

        return true;
    }

    void ConfigPanel::DisplayGUI()
    {
        assert(p_Config != nullptr || "Config not initialized!");

        if (!p_Config->showConfigPanel)
        {
            return;
        }

        bool show = true;
        ImGui::ShowDemoWindow(&show);

        DisplayMainMenu();
        DisplayPipelineWindow();
        DisplayConfigWindow(*p_Config);
        DisplayControlsOverlay();
        DisplayMetricsOverlay();
    }

    void ConfigPanel::DisplayMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            // Menu items
            // ----------------
            if (ImGui::MenuItem("Open scene"))
            {
                IGFD::FileDialogConfig config;
                ImGuiFileDialog::Instance()->OpenDialog("OpenSceneFile", "Choose model (.obj)", ".obj", config);
            }

            if (ImGui::MenuItem("Scene Editor"))
            {
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
            if (ImGuiFileDialog::Instance()->Display("OpenSceneFile")) {
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

    void ConfigPanel::DisplayPipelineWindow()
    {
        // TODO: disable resizing below min size?
        // TODO: open separate windows by clicking
		if (ImGui::Begin("Graphics pipeline"))
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			static const char* stages[] = {
				"Input Assembler", "Vertex Shader", "Tessellation",
				"Geometry Shader", "Rasterization",
				"Fragment Shader", "Color Blending"
			};
			int numStages = sizeof(stages) / sizeof(stages[0]);

			ImVec2 windowPos = ImGui::GetWindowPos();  // Window top-left corner
            ImVec2 windowSize = ImGui::GetWindowSize();
            windowSize.y -= ImGui::GetFrameHeight();
			windowPos.y += ImGui::GetFrameHeight();
			constexpr ImVec2 padding(40, 0);
            ImVec2 boxSize(
                ((windowSize.x - (numStages + 1) * padding.x) / numStages),
                50
            );
			constexpr ImU32 arrowColor = IM_COL32(255, 255, 255, 255);

			ImVec2 startPos = ImVec2(
                windowPos.x + padding.x, 
                windowPos.y + windowSize.y / 2 - boxSize.y / 2
            );

			for (int i = 0; i < numStages; ++i) 
            {
				ImVec2 boxMin = ImVec2(startPos.x + i * (boxSize.x + padding.x), startPos.y);
				ImVec2 boxMax = ImVec2(boxMin.x + boxSize.x, boxMin.y + boxSize.y);
				ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(ImVec4(ImColor::HSV(i / (float)numStages, 0.6f, 0.6f)));

				drawList->AddRectFilled(boxMin, boxMax, boxColor, 5.0f);  // Rounded box
				drawList->AddRect(boxMin, boxMax, IM_COL32_BLACK);  // Border

				ImVec2 text_pos = ImVec2(boxMin.x + 10, boxMin.y + 15);
				drawList->AddText(text_pos, IM_COL32_BLACK, stages[i]);

				if (i < numStages - 1) 
                {
					ImVec2 arrowStart = ImVec2(boxMax.x, boxMin.y + boxSize.y / 2);
					ImVec2 arrowEnd = ImVec2(arrowStart.x + padding.x, arrowStart.y);

					// Arrow line
					drawList->AddLine(arrowStart, arrowEnd, arrowColor, 2.0f);

					// Arrow head
					ImVec2 arrow_head1 = ImVec2(arrowEnd.x - 5, arrowEnd.y - 5);
					ImVec2 arrow_head2 = ImVec2(arrowEnd.x - 5, arrowEnd.y + 5);
					drawList->AddTriangleFilled(arrowEnd, arrow_head1, arrow_head2, arrowColor);
				}
			}
		}
		ImGui::End();
    }

    void ConfigPanel::DisplayConfigWindow(Config& config) 
    {
        if (ImGui::Begin("Configuration")) {
            if (ImGui::BeginTabBar("Camera"))
            {
                if (ImGui::BeginTabItem("Camera"))
                {
                    ImGui::Text("Camera settings");
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Renderer"))
                {
                    DisplayRendererSettings();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Environment"))
                {
                    DisplayEnvironmentSettings();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Culling"))
                {
                    ImGui::Text("Culling settings");
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void ConfigPanel::DisplayCameraSettings()
    {

    }

    void ConfigPanel::DisplayRendererSettings()
    {
        auto &r = p_Config->renderer;
        ImGui::SeparatorText("Renderer settings");
        if (ImGui::Button("Take screenshot"))
        {
            // TODO: implement screenshot
            LOGD("Taking screenshot");
        }
        ImGui::SeparatorText("Scene");
        ImGui::Checkbox("Show wireframe", &r.showWireframe);
        ImGui::Checkbox("Enable perspective-correct interpolation", &r.enablePerspectiveCorrect);
        const char* aaItems[] = { "None", "MSAA", "FXAA" };
        ImGui::Combo("Anti-aliasing", reinterpret_cast<int *>(&r.aaType), aaItems, IM_ARRAYSIZE(aaItems));
        ImGui::ColorEdit4("Clear screen color", reinterpret_cast<float*>(&r.clearColor), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        ImGui::Text("Background color:");
    }

    void ConfigPanel::DisplayEnvironmentSettings()
    {
        auto &e = p_Config->environment;
        ImGui::SeparatorText("Environment settings");
        ImGui::Checkbox("Show skybox", &e.showSkybox);
        ImGui::Checkbox("Show grid", &e.showGrid);
        ImGui::Checkbox("Show floor", &e.showFloor);
        ImGui::Checkbox("Shadow mapping", &e.shadowMap);
    }

    void ConfigPanel::DisplayControlsOverlay()
    {
        static bool isOpen = true;
        if (!isOpen) return;

        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        constexpr float kPadding = 10.0f;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = work_pos.x + kPadding;
        window_pos.y = work_pos.y + kPadding;
        window_pos_pivot.x = 0.0f;
        window_pos_pivot.y = 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        windowFlags |= ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("Controls", &isOpen, windowFlags))
        {
            ImGui::Text("%c - toggle GUI", GetKey(InputAction::TOGGLE_CONFIG_PANEL));
            ImGui::Text("%c - show wireframe", GetKey(InputAction::TOGGLE_WIREFRAME));
        }
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