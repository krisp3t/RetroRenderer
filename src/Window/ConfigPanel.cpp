/***
 * Wrapper around Dear ImGui to create a configuration panel for the renderer.
 * Also displays rendered image in an ImGui window.
 */
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <SDL.h>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

#include "../../lib/ImGuiFileDialog/ImGuiFileDialog.h"
#include "ConfigPanel.h"
#include "../Base/Logger.h"
#include "../Base/InputActions.h"
#include "../Base/Event.h"
#include "../Engine.h"

namespace RetroRenderer
{
    ConfigPanel::ConfigPanel(SDL_Window *window,
                             SDL_GLContext glContext,
                             std::shared_ptr<Config> config,
                             std::shared_ptr<Camera> camera,
                             const char* glslVersion,
                             std::shared_ptr<Stats> stats
    )
    {
        Init(window, glContext, config, camera, glslVersion, stats);
    }

    ConfigPanel::~ConfigPanel()
    {
        Destroy();
    }

    bool ConfigPanel::Init(SDL_Window* window,
                           SDL_GLContext glContext,
                           std::shared_ptr<Config> config,
                           std::shared_ptr<Camera> camera,
                           const char* glslVersion,
                           std::shared_ptr<Stats> stats
                           )
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard
                        | ImGuiConfigFlags_DockingEnable;
        // TODO: add ini
		//io.IniFilename = "config_panel.ini";
        StyleColorsEnemymouse();
		io.Fonts->AddFontFromFileTTF("assets/fonts/Tomorrow-Italic.ttf", 20);
        ImGui_ImplSDL2_InitForOpenGL(window, glContext);
        ImGui_ImplOpenGL3_Init(glslVersion);

        m_Window = window;
        p_Config = config;
        p_Camera = camera;
        p_Stats = stats;

        return true;
    }

    void ConfigPanel::StyleColorsEnemymouse()
    {
        // Theme from @enemymouse
        // https://gist.github.com/enemymouse/c8aa24e247a1d7b9fc33d45091cbb8f0
        // https://github.com/GraphicsProgramming/dear-imgui-styles
		ImGuiStyle& style = ImGui::GetStyle();
		style.Alpha = 1.0;
		//style.WindowFillAlpha = 0.83;
		style.ChildRounding = 3;
		style.WindowRounding = 3;
		style.GrabRounding = 1;
		style.GrabMinSize = 20;
		style.FrameRounding = 3;


		style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.40f, 0.41f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 1.00f, 1.00f, 0.65f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.80f, 0.80f, 0.18f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.44f, 0.80f, 0.80f, 0.27f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.81f, 0.86f, 0.66f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.21f, 0.73f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.27f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.22f, 0.29f, 0.30f, 0.71f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.44f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
		//style.Colors[ImGuiCol_ComboBg] = ImVec4(0.16f, 0.24f, 0.22f, 0.60f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 1.00f, 0.33f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
		//style.Colors[ImGuiCol_Column] = ImVec4(0.00f, 0.50f, 0.50f, 0.33f);
		//style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.00f, 0.50f, 0.50f, 0.47f);
		//style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.00f, 0.70f, 0.70f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
		//style.Colors[ImGuiCol_CloseButton] = ImVec4(0.00f, 0.78f, 0.78f, 0.35f);
		//style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.00f, 0.78f, 0.78f, 0.47f);
		//style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.00f, 0.78f, 0.78f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);
		//style.Colors[ImGuiCol_TooltipBg] = ImVec4(0.00f, 0.13f, 0.13f, 0.90f);
		//style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.04f, 0.10f, 0.09f, 0.51f);
    }

    void ConfigPanel::DisplayGUI()
    {
        assert(p_Config != nullptr || "Config not initialized!");
        if (!p_Config->showConfigPanel)
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(),
                                         ImGuiDockNodeFlags_PassthruCentralNode);
        }

        //bool show = true;
        //ImGui::ShowDemoWindow(&show);

        DisplayMainMenu();
        //DisplayPipelineWindow();
        // TODO: add examples file browser
        DisplaySceneGraph();
        DisplayConfigWindow(*p_Config);
        DisplayControlsOverlay();
        DisplayMetricsOverlay();
    }

    void ConfigPanel::DisplaySceneGraph()
    {
        ImGui::Begin("Scene Graph");
        //ImGui::Text("Please load a scene!");
		if (ImGui::TreeNode("Main camera"))
		{
			ImGui::Text("Main camera");
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Ambient light"))
		{
			ImGui::Text("Ambient light");
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Scene"))
		{
			if (ImGui::TreeNode("Test Node"))
			{
				ImGui::Text("Ambient light");
				ImGui::TreePop();
			}
			ImGui::Text("Scene root");
			ImGui::TreePop();
		}
        /*
        ImGui::TreeNode("Model 1");
		ImGui::TreePop();
		ImGui::TreeNode("Model 3");
		ImGui::TreePop();
        */
        ImGui::End();
    }

    void ConfigPanel::DisplayRenderedImage()
    {
		ImGui::Begin("Output");
		ImGui::Text("Please load a scene to start rendering!");
		ImGui::End();
    }

    void ConfigPanel::DisplayRenderedImage(GLuint p_framebufferTexture)
    {
		ImGui::Begin("Output");
		ImVec2 contentSize = ImGui::GetContentRegionAvail();

        auto ReleaseMouse = [&]()
            {
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 windowSize = ImGui::GetWindowSize();
                ImVec2 resetPos = ImVec2(windowPos.x + windowSize.x / 2, windowPos.y + windowSize.y / 2);
                LOGD("Stop camera drag, resetting mouse to (%.0f, %.0f)", resetPos.x, resetPos.y);
                m_isDragging = false;
                SDL_SetRelativeMouseMode(SDL_FALSE);
                SDL_WarpMouseInWindow(m_Window, resetPos.x, resetPos.y); // reset to center
            };

		auto HandleCameraDrag = [&]()
			{
				int deltaX, deltaY;
				SDL_GetRelativeMouseState(&deltaX, &deltaY);
				p_Camera->eulerRotation.y += deltaX * 0.05f;
				p_Camera->eulerRotation.x -= deltaY * 0.05f;
			};

        if (ImGui::IsWindowHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            // TODO: emit event instead?

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
				if (!m_isDragging)
				{
                    LOGD("Start camera drag");
                    m_isDragging = true;
                    SDL_SetRelativeMouseMode(SDL_TRUE); // Capture mouse
				}
                HandleCameraDrag();
            }
            // Mouse released when hovering, stop dragging
			else if (m_isDragging)
			{
                ReleaseMouse();
			}

            // Zoom camera (forward/backward along forward vector)
			if (ImGui::GetIO().MouseWheel != 0.0f)
			{
				glm::vec3& forward = p_Camera->direction;
				p_Camera->position += forward * ImGui::GetIO().MouseWheel * 0.1f;
			}
        }
        else if (m_isDragging) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                HandleCameraDrag();
            }
            else
            {
                ReleaseMouse();
            }
        }
         
        ImGui::Image((void*)(intptr_t)p_framebufferTexture, contentSize);
		ImGui::End();
    }

    /*
    // Pan camera (sideways and up/down relative to direction)
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        glm::vec3& forward = p_Camera->direction;
        glm::vec3 right = glm::normalize(glm::cross(forward, p_Camera->up));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        p_Camera->position += right * -ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).x * 0.005f;
        p_Camera->position += up * ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).y * 0.005f;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
    }
    // Dolly camera
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        glm::vec3& forward = p_Camera->direction;
        p_Camera->position += forward * ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).y * 0.01f;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
    }
    */

    void ConfigPanel::DisplayMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            // Menu items
            // ----------------
            if (ImGui::MenuItem("Open scene"))
            {
                IGFD::FileDialogConfig config;
                ImGuiFileDialog::Instance()->OpenDialog("OpenSceneFile", "Choose scene", ".obj,.gltf,.glb,.fbx,.usd", config);
            }

            if (ImGui::MenuItem("Scene Editor"))
            {
            }

            if (ImGui::MenuItem("Reset"))
            {
				Engine::Get().DispatchImmediate(SceneResetEvent{});
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
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    LOGD("Selected model file: %s", filePathName.c_str());
					Engine::Get().DispatchImmediate(SceneLoadEvent{ std::move(filePathName) });
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
                    DisplayCameraSettings();
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
				if (ImGui::BeginTabItem("Rasterization"))
				{
                    DisplayRasterizerSettings();
					ImGui::EndTabItem();
				}
                if (ImGui::BeginTabItem("Post-FX"))
                {
                    ImGui::Text("Post-FX");
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void ConfigPanel::DisplayCameraSettings()
    {
        if (p_Camera)
        {
            ImGui::SeparatorText("Camera settings");
            ImGui::DragFloat3("Position", glm::value_ptr(p_Camera->position), 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
			ImGui::DragFloat3("Rotation", glm::value_ptr(p_Camera->eulerRotation), 0.1f, -180.0f, 180.0f, "%.3f");
            ImGui::Combo("Camera type", reinterpret_cast<int *>(&p_Camera->type), "Perspective\0Orthographic\0");
            ImGui::SliderFloat("Field of view", &p_Camera->fov, 1.0f, 179.0f);
            ImGui::SliderFloat("Near plane", &p_Camera->near, 0.1f, 10.0f);
            ImGui::SliderFloat("Far plane", &p_Camera->far, 1.0f, 100.0f);
            ImGui::SliderFloat("Orthographic size", &p_Camera->orthoSize, 1.0f, 100.0f);
        }
        else
        {
            ImGui::Text("No camera available");
        }

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
        ImGui::Checkbox("Enable perspective-correct interpolation", &r.enablePerspectiveCorrect);
        const char* aaItems[] = { "None", "MSAA", "FXAA" };
        ImGui::Combo("Anti-aliasing", reinterpret_cast<int *>(&r.aaType), aaItems, IM_ARRAYSIZE(aaItems));
        ImGui::ColorEdit4("Clear screen color", reinterpret_cast<float*>(&r.clearColor), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        ImGui::Text("Background color:");
        ImGui::InputInt2("Viewport resolution", reinterpret_cast<int*>(&r.viewportResolution)); // TODO: add min, max
    }

    void ConfigPanel::DisplayRasterizerSettings()
    {
        auto& r = p_Config->rasterizer;
        ImGui::SeparatorText("Rasterizer settings");

        const char* lineItems[] = { "DDA (slower)", "Bresenham (faster)"};
        ImGui::Combo("Line mode", reinterpret_cast<int*>(&r.lineMode), lineItems, IM_ARRAYSIZE(lineItems));
		const char* polyItems[] = { "Point", "Wireframe (line)", "Fill triangles" };
		ImGui::Combo("Polygon mode", reinterpret_cast<int*>(&r.polygonMode), polyItems, IM_ARRAYSIZE(polyItems));

        ImGui::SeparatorText("Point");
		ImGui::SliderFloat("Point size", &r.pointSize, 1.0f, 10.0f);
        ImGui::ColorEdit4("Point color", reinterpret_cast<float*>(&r.lineColor), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

        ImGui::SeparatorText("Wireframe");
		ImGui::SliderFloat("Line width", &r.lineWidth, 1.0f, 10.0f);
        ImGui::ColorEdit4("Line color", reinterpret_cast<float*>(&r.lineColor), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::Checkbox("Display triangle edges as RGB", &r.basicLineColors);

        ImGui::SeparatorText("Fill");
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
        window_pos.x = work_pos.x + work_size.x - kPadding;
        window_pos.y = work_pos.y + kPadding;
        window_pos_pivot.x = 1.0f;
        window_pos_pivot.y = 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        windowFlags |= ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("Controls", &isOpen, windowFlags))
        {
            ImGui::Text("h - toggle GUI");
            ImGui::Text("1 - show wireframe");
        }
        ImGui::End();
    }

    void ConfigPanel::DisplayMetricsOverlay()
    {
        static bool isOpen = true;
        if (!isOpen) return;

        static int location = 2;
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
            if (p_Camera)
            {
                ImGui::Text("Camera position: (%.3f, %.3f, %.3f)", p_Camera->position.x, p_Camera->position.y, p_Camera->position.z);
            }
			assert(p_Stats != nullptr && "Stats not initialized!");
            ImGui::Text("%d verts, %d tris", p_Stats->renderedVerts, p_Stats->renderedTris);
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

    void ConfigPanel::BeforeFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        DisplayGUI();
    }

    void ConfigPanel::OnDraw()
    {
        ImGui::Render();
		auto io = ImGui::GetIO();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        auto c = p_Config->renderer.clearColor;
		glClearColor(c.x * c.w, c.y * c.w, c.z * c.w, c.w);
		glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		// Multi-viewport support
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backupCurrentWindow = SDL_GL_GetCurrentWindow();
            SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
        }
    }

    void ConfigPanel::Destroy()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
}