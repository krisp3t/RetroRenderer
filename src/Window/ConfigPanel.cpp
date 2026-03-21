/***
 * Wrapper around Dear ImGui to create a configuration panel for the renderer.
 * Also displays rendered image in an ImGui window.
 */
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#ifdef __EMSCRIPTEN__
#include "../native/emscripten/EmscriptenBridge.h"
#include "../native/emscripten/imgui_impl_sdl2.h"
#include <emscripten.h>
#else
#include <imgui_impl_sdl2.h>
#endif
#include "../../lib/ImGuiFileDialog/ImGuiFileDialog.h"
#include <KrisLogger/Logger.h>
#include <SDL.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <cinttypes>
#include <utility>

#ifdef __ANDROID__
#include "../native/AndroidBridge.h"
#endif

#include "../Base/Event.h"
#include "../Base/InputActions.h"
#include "../Engine.h"
#include "../Renderer/RetroPalette.h"
#include "ConfigPanel.h"

#ifdef __EMSCRIPTEN__
EM_JS(void, OpenWebFilePicker_JS, (), {
    let input = document.createElement('input');
    input.type = 'file';
    input.accept = '.obj';
    input.style.display = 'none';

    input.onchange = e => {
        let file = e.target.files[0];
        if (!file)
            return;

        let reader = new FileReader();
        reader.onload = function(event) {
            let data = event.target.result;

            // Allocate memory in WASM and copy file
            let size = data.byteLength;
            let ptr = Module._malloc(size);
            if (!ptr) {
                console.error("Failed to allocate memory");
                return;
            }
            let heapBytes = new Uint8Array(Module.HEAPU8.buffer, ptr, size);
            heapBytes.set(new Uint8Array(data));

            Module.ccall('OnWebFileSelected', null, [ 'number', 'number' ], [ ptr, size ]);
        };
        reader.readAsArrayBuffer(file);
    };

    document.body.appendChild(input);
    input.click();
    document.body.removeChild(input);
});
#endif

namespace RetroRenderer {
namespace {
const char* RenderPresetDescription(Config::RenderPreset preset) {
    switch (preset) {
    case Config::RenderPreset::CUSTOM:
        return "Manual configuration. Use this when you want to tune the renderer without a preset overriding the baseline.";
    case Config::RenderPreset::DEFAULT:
        return "Baseline renderer settings with the current window-sized render target and standard presentation filtering.";
    case Config::RenderPreset::PICO8:
        return "Low-resolution software preset aimed at palette reduction, color ramps, and ordered dithering.";
    case Config::RenderPreset::PICOCAD:
        return "Low-resolution software preset aimed at picoCAD-style low-poly rendering, flat face lighting, reduced textures, texture-derived palettes, affine mapping, and vertex snap.";
    }
    return "";
}

ImVec4 PixelToImVec4(const Pixel& pixel) {
    return {
        static_cast<float>(pixel.r) / 255.0f,
        static_cast<float>(pixel.g) / 255.0f,
        static_cast<float>(pixel.b) / 255.0f,
        static_cast<float>(pixel.a) / 255.0f,
    };
}

Pixel ImVec4ToOpaquePixel(const ImVec4& color) {
    return Pixel{
        static_cast<uint8_t>(std::clamp(color.x, 0.0f, 1.0f) * 255.0f),
        static_cast<uint8_t>(std::clamp(color.y, 0.0f, 1.0f) * 255.0f),
        static_cast<uint8_t>(std::clamp(color.z, 0.0f, 1.0f) * 255.0f),
        255,
    };
}

void DrawPalettePreviewGrid(const std::array<Color, RetroPalette::kPico8PaletteSize>& paletteColors) {
    for (size_t i = 0; i < paletteColors.size(); i++) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::ColorButton("##palettePreview", paletteColors[i].ToImVec4(), ImGuiColorEditFlags_NoTooltip, ImVec2(28.0f, 28.0f));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%02zu  #%02X%02X%02X", i, paletteColors[i].r, paletteColors[i].g, paletteColors[i].b);
        }
        ImGui::PopID();
        if ((i & 3) != 3) {
            ImGui::SameLine();
        }
    }
}

bool DrawCustomPaletteEditor(Config::RetroStyleSettings& retro) {
    bool changed = false;
    for (size_t i = 0; i < retro.customPalette.size(); i++) {
        ImVec4 color = PixelToImVec4(retro.customPalette[i]);
        float colorValues[3] = {color.x, color.y, color.z};
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::ColorEdit3(
                "##customPaletteColor",
                colorValues,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha |
                    ImGuiColorEditFlags_DisplayRGB)) {
            retro.customPalette[i] = ImVec4ToOpaquePixel(ImVec4(colorValues[0], colorValues[1], colorValues[2], 1.0f));
            retro.customPaletteRevision++;
            changed = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "%02zu  #%02X%02X%02X",
                i,
                retro.customPalette[i].r,
                retro.customPalette[i].g,
                retro.customPalette[i].b);
        }
        ImGui::PopID();
        if ((i & 3) != 3) {
            ImGui::SameLine();
        }
    }
    return changed;
}
} // namespace

ConfigPanel::~ConfigPanel() {
    Destroy();
}

bool ConfigPanel::Init(SDL_Window* window, SDL_GLContext glContext, std::shared_ptr<Config> config,
                       const char* glslVersion, std::shared_ptr<Stats> stats) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
#ifdef __EMSCRIPTEN__
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    ImGui::GetIO().DisplaySize = ImVec2((float)w, (float)h);
#endif

#ifdef __ANDROID__
    io.IniFilename = g_imguiIniPath.c_str();
#else
    io.IniFilename = "assets/config_panel.ini";
#endif
    StyleColorsEnemymouse();
#ifdef __ANDROID__
    AAsset* font_asset = AAssetManager_open(g_assetManager, "fonts/Tomorrow-Italic.ttf", AASSET_MODE_BUFFER);
    if (font_asset) {
        size_t fileSize = AAsset_getLength(font_asset);
        m_fontData_.resize(fileSize);
        AAsset_read(font_asset, m_fontData_.data(), fileSize);
        AAsset_close(font_asset);
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false; // we keep ownership in m_fontData_
        io.Fonts->AddFontFromMemoryTTF(m_fontData_.data(), (int)m_fontData_.size(), 32.0f, &cfg);
    }
#else
    io.Fonts->AddFontFromFileTTF("assets/fonts/Tomorrow-Italic.ttf", 20);
#endif
    // io.FontGlobalScale = 2.0f;
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

    p_Window_ = window;
    p_config_ = config;
    p_stats_ = stats;

    return true;
}

void ConfigPanel::ApplyRendererPreset(Config::RenderPreset preset) {
    if (!p_config_) {
        return;
    }

    Config::ApplyRenderPreset(*p_config_, preset);
    Engine::Get().DispatchImmediate(OutputImageResizeEvent{p_config_->renderer.resolution});
}

void ConfigPanel::MarkRendererPresetCustom() {
    if (!p_config_ || p_config_->retro.preset == Config::RenderPreset::CUSTOM) {
        return;
    }
    p_config_->retro.preset = Config::RenderPreset::CUSTOM;
}

void ConfigPanel::StyleColorsEnemymouse() {
    // Theme from @enemymouse
    // https://gist.github.com/enemymouse/c8aa24e247a1d7b9fc33d45091cbb8f0
    // https://github.com/GraphicsProgramming/dear-imgui-styles
    ImGuiStyle& style = ImGui::GetStyle();
    style.Alpha = 1.0;
    // style.WindowFillAlpha = 0.83;
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
    // style.Colors[ImGuiCol_ComboBg] = ImVec4(0.16f, 0.24f, 0.22f, 0.60f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 1.00f, 0.33f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    // style.Colors[ImGuiCol_Column] = ImVec4(0.00f, 0.50f, 0.50f, 0.33f);
    // style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.00f, 0.50f, 0.50f, 0.47f);
    // style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.00f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    // style.Colors[ImGuiCol_CloseButton] = ImVec4(0.00f, 0.78f, 0.78f, 0.35f);
    // style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.00f, 0.78f, 0.78f, 0.47f);
    // style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.00f, 0.78f, 0.78f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);
    // style.Colors[ImGuiCol_TooltipBg] = ImVec4(0.00f, 0.13f, 0.13f, 0.90f);
    // style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.04f, 0.10f, 0.09f, 0.51f);
}

void ConfigPanel::DisplayGUI() {
    assert(p_config_ != nullptr || "Config not initialized!");
    if (!p_config_->window.showConfigPanel) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    }

    // bool show = true;
    // ImGui::ShowDemoWindow(&show);

    DisplayJoysticks();
    DisplayMainMenu();
    // DisplayPipelineWindow();
    //  TODO: add examples file browser
    DisplaySceneGraph();
    // DisplayInspectorWindow();
    DisplayMaterialWindow();
    DisplayConfigWindow(*p_config_);
    DisplayControlsOverlay();
    DisplayMetricsOverlay();
    DisplayExamplesDialog();
}

void ConfigPanel::DisplayExamplesDialog() {
    if (m_isFileDialogOpen_)
        return;
#ifdef __ANDROID__
    // We only support native file picker on Android.
    return;
#endif
    IGFD::FileDialogConfig examplesDialogConfig;
    examplesDialogConfig.countSelectionMax = 1;
    ImGuiFileDialog::Instance()->OpenDialog("OpenExampleFile", "Examples", k_supportedModels, examplesDialogConfig);
    if (ImGuiFileDialog::Instance()->Display("OpenExampleFile")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            LOGD("Selected example file: %s", filePathName.c_str());
            Engine::Get().DispatchImmediate(SceneLoadEvent{filePathName});
            // IGFD::FileManager::SetCurrentPath("tests-visual/basic-tests/");
        }
    }
}

void ConfigPanel::DisplayWindowSettings() {
    auto& w = p_config_->window;
    ImGui::SeparatorText("Window settings");
    // ImGui::Checkbox("Show configuration panel", &w.showConfigPanel);
    ImGui::Checkbox("Show controls", &w.showControls);
    ImGui::Checkbox("Show FPS", &w.showFPS);
    if (ImGui::Checkbox("Enable VSync", &w.enableVsync)) {
        SDL_GL_SetSwapInterval(w.enableVsync ? 1 : 0);
    }
}

void ConfigPanel::DisplaySceneGraph() {
    ImGui::Begin("Scene Graph");
    ImGuiIO& io = ImGui::GetIO();
    ImFont* smallFont = io.Fonts->Fonts[0]; // base font (index 0)
    constexpr float kScale = 0.8f;
    ImGui::PushFont(smallFont);
    ImGui::SetWindowFontScale(kScale);

    Engine::Get().GetSceneManager().RenderUI();

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::End();
}

void ConfigPanel::DisplayInspectorWindow() {
    ImGui::Begin("Inspector");
    ImGui::Text("Inspector");
    ImGui::End();
}

void ConfigPanel::OnDraw(GLuint framebufferTexture) {
}

void ConfigPanel::DisplayMaterialWindow() {
    ImGui::Begin("Material Editor");
    Engine::Get().GetMaterialManager().RenderUI();
    ImGui::End();
}

void ConfigPanel::DisplayRenderedImage() {
    ImGui::Begin("Output");
    ImGui::Text("Please load a scene to start rendering!");
    ImGui::End();
}

void ConfigPanel::DisplayRenderedImage(GLuint p_framebufferTexture) {
    auto& r = p_config_->renderer;
    ImGui::Begin("Output");
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    glm::ivec2 displaySize = {contentSize.x * r.resolutionScale, contentSize.y * r.resolutionScale};
    if (displaySize.x > 0 && displaySize.y > 0 && r.resolution != displaySize) {
        Engine::Get().DispatchImmediate(OutputImageResizeEvent{displaySize});
    }

    auto ReleaseMouse = [&]() {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 resetPos = ImVec2(windowPos.x + windowSize.x / 2, windowPos.y + windowSize.y / 2);
        LOGD("Stop camera drag, resetting mouse to (%.0f, %.0f)", resetPos.x, resetPos.y);
        m_isDragging_ = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_WarpMouseInWindow(p_Window_, resetPos.x, resetPos.y); // reset to center
    };

    auto HandleCameraDrag = [&]() {
        int deltaX = 0;
        int deltaY = 0;
        SDL_GetRelativeMouseState(&deltaX, &deltaY);
        if (auto cam = Engine::Get().GetCamera()) {
            cam->m_EulerRotation.y += deltaX * 0.05f;
            cam->m_EulerRotation.x -= deltaY * 0.05f;
        }
    };

    ImGuiIO& io = ImGui::GetIO();

    // Get mouse position relative to the window's content area
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin =
        ImVec2(windowPos.x + ImGui::GetWindowContentRegionMin().x, windowPos.y + ImGui::GetWindowContentRegionMin().y);
    ImVec2 contentMax =
        ImVec2(windowPos.x + ImGui::GetWindowContentRegionMax().x, windowPos.y + ImGui::GetWindowContentRegionMax().y);
    ImVec2 mousePos = io.MousePos;

    // Is the mouse INSIDE the content area (not title bar/borders)?
    bool isMouseInContent = (mousePos.x >= contentMin.x && mousePos.x <= contentMax.x && mousePos.y >= contentMin.y &&
                             mousePos.y <= contentMax.y);

    // Only activate camera if:
    // 1. Mouse is in content area
    // 2. "Output" window is hovered
    // 3. ImGui isn't using the mouse for other UI
    bool isViewportActive = isMouseInContent && ImGui::IsWindowHovered();

    if (auto cam = Engine::Get().GetCamera()) {
        if (isViewportActive) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            // TODO: emit event instead?

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (!m_isDragging_) {
                    LOGD("Start camera drag");
                    m_isDragging_ = true;
                    SDL_SetRelativeMouseMode(SDL_TRUE); // Capture mouse
                }
                HandleCameraDrag();
            }
            // Mouse released when hovering, stop dragging
            else if (m_isDragging_) {
                ReleaseMouse();
            }

            // Zoom camera (forward/backward along forward vector)
            if (ImGui::GetIO().MouseWheel != 0.0f) {
                glm::vec3& forward = cam->m_Direction;
                cam->m_Position += forward * ImGui::GetIO().MouseWheel * 0.1f;
            }
        } else if (m_isDragging_) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                HandleCameraDrag();
            } else {
                ReleaseMouse();
            }
        }
    }
    switch (p_config_->renderer.selectedRenderer) {
    case Config::RendererType::SOFTWARE:
        ImGui::Image(p_framebufferTexture, contentSize);
        break;
    case Config::RendererType::GL:
        // OpenGL textures are flipped vertically
        ImGui::Image(p_framebufferTexture, contentSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        break;
    default:
        ImGui::Text("Renderer type %d not implemented!", p_config_->renderer.selectedRenderer);
        break;
    }

    ImGui::End();
}

/*
// Pan camera (sideways and up/down relative to direction)
if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
{
    glm::vec3& forward = p_camera_->direction;
    glm::vec3 right = glm::normalize(glm::cross(forward, p_camera_->up));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    p_camera_->position += right * -ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).x * 0.005f;
    p_camera_->position += up * ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).y * 0.005f;
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
}
// Dolly camera
if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
{
    glm::vec3& forward = p_camera_->direction;
    p_camera_->position += forward * ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).y * 0.01f;
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
}
*/

void ConfigPanel::DisplayMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        // Menu items
        // ----------------
        if (ImGui::MenuItem("Open scene")) {
#if defined(__ANDROID__)
            OpenAndroidFilePicker();
#elif defined(__EMSCRIPTEN__)
            OpenWebFilePicker();
#else
            IGFD::FileDialogConfig sceneDialogConfig;
            ImGuiFileDialog::Instance()->OpenDialog("OpenSceneFile", "Choose scene", k_supportedModels,
                                                    sceneDialogConfig);
#endif
        }
        if (ImGuiFileDialog::Instance()->Display("OpenTextureFile")) {
            m_isFileDialogOpen_ = true;
            if (ImGuiFileDialog::Instance()->IsOk()) {
                m_isFileDialogOpen_ = false;
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                LOGD("Selected texture file: %s", filePathName.c_str());
                Engine::Get().GetMaterialManager().LoadTexture(filePathName);

                // TODO: event needed?
                // Engine::Get().DispatchImmediate(TextureLoadEvent{filePathName});
            }
            ImGuiFileDialog::Instance()->Close();
        } else {
            m_isFileDialogOpen_ = false;
        }

        if (ImGui::MenuItem("Reset")) {
            Engine::Get().DispatchImmediate(SceneResetEvent{});
        }

        if (ImGui::MenuItem("About")) {
            ImGui::OpenPopup("About");
        }

        // Dialog windows
        // ----------------
        // Open Model File dialog
        if (ImGuiFileDialog::Instance()->Display("OpenSceneFile")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                LOGD("Selected model file: %s", filePathName.c_str());
                Engine::Get().DispatchImmediate(SceneLoadEvent{filePathName});
            }
            ImGuiFileDialog::Instance()->Close();
        }
        // Open Texture File dialog

        // About dialog
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            // TODO: Add version number
            ImGui::TextWrapped("RetroRenderer v%d.%d", 0, 1);
            ImGui::TextWrapped("Software renderer with fixed-function pipeline for educational purposes.");
            ImGui::Separator();
            ImGui::TextWrapped("Built by krisp3t under the MIT License. Enjoy!");

            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 120.0f) * 0.5f);
            ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 30.0f);
            if (ImGui::Button("Close", ImVec2(120.0f, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::EndMainMenuBar();
    }
}

void ConfigPanel::DisplayPipelineWindow() {
    // TODO: disable resizing below min size?
    // TODO: open separate windows by clicking
    if (ImGui::Begin("Graphics pipeline")) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        static const char* stages[] = {"Input Assembler", "Vertex Shader",   "Tessellation",  "Geometry Shader",
                                       "Rasterization",   "Fragment Shader", "Color Blending"};
        int numStages = sizeof(stages) / sizeof(stages[0]);

        ImVec2 windowPos = ImGui::GetWindowPos(); // Window top-left corner
        ImVec2 windowSize = ImGui::GetWindowSize();
        windowSize.y -= ImGui::GetFrameHeight();
        windowPos.y += ImGui::GetFrameHeight();
        constexpr ImVec2 padding(40, 0);
        ImVec2 boxSize(((windowSize.x - (numStages + 1) * padding.x) / numStages), 50);
        constexpr ImU32 arrowColor = IM_COL32(255, 255, 255, 255);

        ImVec2 startPos = ImVec2(windowPos.x + padding.x, windowPos.y + windowSize.y / 2 - boxSize.y / 2);

        for (int i = 0; i < numStages; ++i) {
            ImVec2 boxMin = ImVec2(startPos.x + i * (boxSize.x + padding.x), startPos.y);
            ImVec2 boxMax = ImVec2(boxMin.x + boxSize.x, boxMin.y + boxSize.y);
            ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(ImVec4(ImColor::HSV(i / (float)numStages, 0.6f, 0.6f)));

            drawList->AddRectFilled(boxMin, boxMax, boxColor, 5.0f); // Rounded box
            drawList->AddRect(boxMin, boxMax, IM_COL32_BLACK);       // Border

            ImVec2 text_pos = ImVec2(boxMin.x + 10, boxMin.y + 15);
            drawList->AddText(text_pos, IM_COL32_BLACK, stages[i]);

            if (i < numStages - 1) {
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

void ConfigPanel::DisplayConfigWindow(Config& config) {
    if (ImGui::Begin("Configuration")) {
        if (ImGui::BeginTabBar("Camera")) {
            if (ImGui::BeginTabItem("Camera")) {
                DisplayCameraSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Window")) {
                DisplayWindowSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Renderer")) {
                DisplayRendererSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Environment")) {
                DisplayEnvironmentSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Culling")) {
                DisplayCullSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Rasterization")) {
                DisplayRasterizerSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Post-FX")) {
                DisplayPostFxSettings();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void ConfigPanel::DisplayCameraSettings() {
    if (auto cam = Engine::Get().GetCamera()) {
        ImGui::SeparatorText("Camera settings");
        ImGui::DragFloat3("Position", glm::value_ptr(cam->m_Position), 0.1f, 0.0f, 0.0f, "%.3f");
        ImGui::DragFloat3("Rotation", glm::value_ptr(cam->m_EulerRotation), 0.1f, -180.0f, 180.0f, "%.3f");
        ImGui::Combo("Camera type", reinterpret_cast<int*>(&cam->m_Type), "Perspective\0Orthographic\0");
        switch (cam->m_Type) {
        case CameraType::PERSPECTIVE:
            ImGui::SliderFloat("Field of view", &cam->m_Fov, 1.0f, 179.0f);
            ImGui::SliderFloat("Near plane", &cam->m_Near, 0.1f, 10.0f);
            ImGui::SliderFloat("Far plane", &cam->m_Far, 1.0f, 100.0f);
            break;
        case CameraType::ORTHOGRAPHIC:
            ImGui::SliderFloat("Orthographic size", &cam->m_OrthoSize, 1.0f, 100.0f);
            break;
        }
    } else {
        ImGui::Text("No camera available");
    }
}

void ConfigPanel::DisplayRendererSettings() {
    auto& r = p_config_->renderer;
    bool manualChange = false;
    ImGui::SeparatorText("Renderer settings");
    if (ImGui::Button("Take screenshot")) {
        // TODO: implement screenshot
        LOGD("Taking screenshot");
    }
    ImGui::SameLine();
    if (ImGui::Button("Send to RenderDoc")) {
        // TODO: implement screenshot
        LOGD("Sending to RenderDoc");
    }

    ImGui::SeparatorText("Preset");
    const char* presetItems[] = {"Custom", "Default", "PICO-8", "picoCAD"};
    int presetIndex = static_cast<int>(p_config_->retro.preset);
    if (ImGui::Combo("Render preset", &presetIndex, presetItems, IM_ARRAYSIZE(presetItems))) {
        ApplyRendererPreset(static_cast<Config::RenderPreset>(presetIndex));
    }
    ImGui::SameLine();
    if (ImGui::Button("Reapply preset")) {
        ApplyRendererPreset(p_config_->retro.preset);
    }
    ImGui::TextWrapped("%s", RenderPresetDescription(p_config_->retro.preset));

    if (ImGui::RadioButton("Software", reinterpret_cast<int*>(&r.selectedRenderer),
                           static_cast<int>(Config::RendererType::SOFTWARE))) {
        manualChange = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("OpenGL", reinterpret_cast<int*>(&r.selectedRenderer),
                           static_cast<int>(Config::RendererType::GL))) {
        manualChange = true;
    }

    ImGui::SeparatorText("Resolution");
    ImGui::Text("Render resolution: %d x %d (@ %.1f scale)", static_cast<int>(r.resolution.x),
                static_cast<int>(r.resolution.y), r.resolutionScale);
    if (ImGui::InputFloat("Render resolution scale", reinterpret_cast<float*>(&r.resolutionScale), 0.1f, 0.5f,
                          "%.1f")) {
        manualChange = true;
        r.resolutionScale = glm::clamp(r.resolutionScale, 0.1f, 4.0f);
        LOGD("Changed render resolution scale to %.1f", r.resolutionScale);
        glm::ivec2 newResolution = {static_cast<int>(floor(p_config_->renderer.resolution.x * r.resolutionScale)),
                                    static_cast<int>(floor(p_config_->renderer.resolution.y * r.resolutionScale))};
        Engine::Get().DispatchImmediate(OutputImageResizeEvent{newResolution});
    }

    ImGui::SeparatorText("Scene");
    manualChange |= ImGui::Checkbox("Enable perspective-correct interpolation", &r.enablePerspectiveCorrect);
    const char* aaItems[] = {"None", "MSAA", "FXAA"};
    manualChange |= ImGui::Combo("Anti-aliasing", reinterpret_cast<int*>(&r.aaType), aaItems, IM_ARRAYSIZE(aaItems));
    ImGui::SeparatorText("Presentation");
    if (ImGui::Checkbox("Nearest-neighbor presentation", &r.nearestNeighborPresentation)) {
        manualChange = true;
        Engine::Get().DispatchImmediate(OutputImageResizeEvent{p_config_->renderer.resolution});
    }
    if (ImGui::ColorEdit4("Clear screen color", reinterpret_cast<float*>(&r.clearColor),
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
        manualChange = true;
    }
    ImGui::SameLine();
    ImGui::Text("Background color:");
    // ImGui::InputInt2("Viewport resolution", reinterpret_cast<int*>(&r.viewportResolution)); // TODO: add min, max

    if (manualChange) {
        MarkRendererPresetCustom();
    }
}

void ConfigPanel::DisplayRasterizerSettings() {
    ImGui::SeparatorText("Rasterizer settings");
    bool manualChange = false;

    if (p_config_->renderer.selectedRenderer == Config::RendererType::SOFTWARE) {
        auto& r = p_config_->software.rasterizer;
        const char* lineItems[] = {"DDA (slower)", "Bresenham (faster)"};
        const char* polyItems[] = {"Point", "Wireframe (line)", "Fill triangles"};
        const char* fillItems[] = {"Scanline", "Barycentric", "Pineda (parallel)"};
        manualChange |= ImGui::Combo("Polygon mode", reinterpret_cast<int*>(&r.polygonMode), polyItems, IM_ARRAYSIZE(polyItems));

        switch (r.polygonMode) {
        case Config::RasterizationPolygonMode::POINT:
            ImGui::SeparatorText("Point");
            manualChange |= ImGui::SliderFloat("Point size", &r.pointSize, 1.0f, 10.0f);
            manualChange |= ImGui::ColorEdit4("Point color", reinterpret_cast<float*>(&r.lineColor));
            break;
        case Config::RasterizationPolygonMode::LINE:
            ImGui::SeparatorText("Wireframe");
            manualChange |= ImGui::Combo("Line mode", reinterpret_cast<int*>(&r.lineMode), lineItems, IM_ARRAYSIZE(lineItems));
            manualChange |= ImGui::SliderFloat("Line width", &r.lineWidth, 1.0f, 10.0f);
            manualChange |= ImGui::ColorEdit4("Line color", reinterpret_cast<float*>(&r.lineColor));
            manualChange |= ImGui::Checkbox("Display triangle edges as RGB", &r.basicLineColors);
            break;
        case Config::RasterizationPolygonMode::FILL:
            ImGui::SeparatorText("Fill");
            manualChange |= ImGui::Combo("Fill mode", reinterpret_cast<int*>(&r.fillMode), fillItems, IM_ARRAYSIZE(fillItems));
        }
    } else if (p_config_->renderer.selectedRenderer == Config::RendererType::GL) {
        auto& r = p_config_->gl.rasterizer;
        const char* polyItems[] = {"Point", "Wireframe (line)", "Fill triangles"};
        manualChange |= ImGui::Combo("Polygon mode", reinterpret_cast<int*>(&r.polygonMode), polyItems, IM_ARRAYSIZE(polyItems));
    }

    if (manualChange) {
        MarkRendererPresetCustom();
    }
}

void ConfigPanel::DisplayCullSettings() {
    auto& c = p_config_->cull;
    bool manualChange = false;
    ImGui::SeparatorText("Cull settings");
    manualChange |= ImGui::Checkbox("Backface culling", &c.backfaceCulling);
    manualChange |= ImGui::Checkbox("Depth testing", &c.depthTest);
    ImGui::SeparatorText("Clip settings");
    ImGui::Text("Clipping triangles and pixels outside of screen is essential to rendering.");
    ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0),
                       "Disabling clipping will produce graphical errors, assert fails and undefined behavior.");
    manualChange |= ImGui::Checkbox("Raster clip", &c.rasterClip);
    manualChange |= ImGui::Checkbox("Geometric clip", &c.geometricClip);
    manualChange |= ImGui::Checkbox("Frustum cull", &c.frustumCull);

    if (manualChange) {
        MarkRendererPresetCustom();
    }
}

void ConfigPanel::DisplayEnvironmentSettings() {
    auto& e = p_config_->environment;
    bool presetChange = false;
    ImGui::SeparatorText("Environment settings");
    presetChange |= ImGui::Checkbox("Show skybox", &e.showSkybox);
    presetChange |= ImGui::Checkbox("Show grid", &e.showGrid);
    presetChange |= ImGui::Checkbox("Show floor", &e.showFloor);
    presetChange |= ImGui::Checkbox("Shadow mapping", &e.shadowMap);
    ImGui::TextWrapped("Scene lights now live in the Scene Graph window. Use that to inspect and move lights.");

    if (presetChange) {
        MarkRendererPresetCustom();
    }
}

void ConfigPanel::DisplayPostFxSettings() {
    auto& retro = p_config_->retro;
    bool manualChange = false;
    static int customPalettePresetIndex = 0;
    static constexpr Config::PaletteType kCustomPalettePresets[] = {
        Config::PaletteType::PICO8,
        Config::PaletteType::DB16,
        Config::PaletteType::SWEETIE16,
    };
    static const char* kCustomPalettePresetLabels[] = {
        "PICO-8",
        "DB16",
        "Sweetie-16",
    };

    ImGui::SeparatorText("Retro style");
    ImGui::Text("Preset: %s", Config::RenderPresetLabel(retro.preset));

    const char* paletteItems[] = {"None", "PICO-8", "DB16", "Sweetie-16", "Custom"};
    int paletteIndex = static_cast<int>(retro.palette);
    if (ImGui::Combo("Palette", &paletteIndex, paletteItems, IM_ARRAYSIZE(paletteItems))) {
        retro.palette = static_cast<Config::PaletteType>(paletteIndex);
        manualChange = true;
    }

    if (retro.palette != Config::PaletteType::NONE) {
        ImGui::Text("Active palette preview");
        DrawPalettePreviewGrid(RetroPalette::GetPaletteColors(retro));
    } else {
        ImGui::TextDisabled("No fixed palette selected.");
    }

    if (retro.palette == Config::PaletteType::CUSTOM) {
        ImGui::Spacing();
        ImGui::Text("Custom palette");
        if (ImGui::Combo("Load preset", &customPalettePresetIndex, kCustomPalettePresetLabels, IM_ARRAYSIZE(kCustomPalettePresetLabels))) {
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset custom from preset")) {
            RetroPalette::CopyPaletteToCustom(retro, kCustomPalettePresets[customPalettePresetIndex]);
            manualChange = true;
        }
        manualChange |= DrawCustomPaletteEditor(retro);
    } else if (retro.palette != Config::PaletteType::NONE) {
        if (ImGui::Button("Copy active palette to custom")) {
            RetroPalette::CopyPaletteToCustom(retro, retro.palette);
            retro.palette = Config::PaletteType::CUSTOM;
            manualChange = true;
        }
    }

    manualChange |= ImGui::Checkbox("Enable palette quantization", &retro.enablePalette);
    manualChange |= ImGui::Checkbox("Derive palette from texture", &retro.useTextureDerivedPalette);
    manualChange |= ImGui::Checkbox("Enable color ramps", &retro.enableColorRamps);
    manualChange |= ImGui::Checkbox("Enable ordered dithering", &retro.enableOrderedDithering);
    manualChange |= ImGui::SliderInt("Lighting bands", &retro.lightingBands, 0, 8);
    manualChange |= ImGui::Checkbox("Flat face lighting", &retro.flatFaceLighting);
    manualChange |= ImGui::Checkbox("Use stable untextured base color", &retro.useStableUntexturedBaseColor);
    ImVec4 untexturedBaseColor = retro.untexturedBaseColor.ToImVec4();
    if (ImGui::ColorEdit3("Untextured base color", reinterpret_cast<float*>(&untexturedBaseColor))) {
        retro.untexturedBaseColor = Color(untexturedBaseColor);
        retro.untexturedBaseColor.a = 255;
        manualChange = true;
    }
    manualChange |= ImGui::SliderInt("Texture max dimension", &retro.textureMaxDimension, 0, 64);
    manualChange |= ImGui::Checkbox("Snap projected vertices", &retro.snapVertices);
    manualChange |= ImGui::Checkbox("Affine texture mapping", &retro.affineTextureMapping);

    ImGui::Spacing();
    ImGui::TextWrapped("Palette quantization, texture-derived palettes, reduced texture sampling, flat face lighting, "
                       "ordered dithering, vertex snapping, and affine mapping are active on the software retro path.");
    if (retro.useTextureDerivedPalette) {
        ImGui::TextWrapped("Texture-derived palette overrides the fixed palette when a software-sampled texture has an auto-derived palette.");
    }

    if (manualChange) {
        MarkRendererPresetCustom();
    }
}

void ConfigPanel::DisplayControlsOverlay() {
    if (!p_config_->window.showControls) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoNav;
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
    if (ImGui::Begin("Controls", &p_config_->window.showControls, windowFlags)) {
        ImGui::Text("h - toggle GUI");
        ImGui::Text("1 - show wireframe");
    }
    ImGui::End();
}

void ConfigPanel::DisplayMetricsOverlay() {
    if (!p_config_->window.showFPS)
        return;

    static int location = 0;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoNav;
    constexpr float kPadding = 10.0f;
    ImFont* smallFont = io.Fonts->Fonts[0]; // base font (index 0)
    constexpr float kScale = 0.8f;

    static float frameTimes[100] = {0}; // Buffer for frame times
    static int frameIndex = 0;
    frameTimes[frameIndex] = 1000.0f / io.Framerate; // Store current frame time in milliseconds
    frameIndex = (frameIndex + 1) % 100;             // Wrap index around when it reaches the buffer size

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
    if (ImGui::Begin("Metrics", &p_config_->window.showFPS, windowFlags)) {
        ImGui::PushFont(smallFont);
        ImGui::SetWindowFontScale(kScale);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("%d x %d (%s)", p_config_->renderer.resolution.x, p_config_->renderer.resolution.y,
                    p_config_->renderer.selectedRenderer == Config::RendererType::SOFTWARE ? "software" : "OpenGL");
        ImGui::Text("Preset: %s", Config::RenderPresetLabel(p_config_->retro.preset));
        ImGui::Text("%d verts, %d tris", p_stats_->renderedVerts, p_stats_->renderedTris);
        ImGui::Text("0 draw calls");
        ImGui::SeparatorText("Memory");
        if (p_stats_->processMemorySupported) {
            const double currentMiB = static_cast<double>(p_stats_->processResidentBytes) / (1024.0 * 1024.0);
            const double peakMiB = static_cast<double>(p_stats_->processResidentPeakBytes) / (1024.0 * 1024.0);
            ImGui::Text("RSS: %.2f MiB", currentMiB);
            ImGui::Text("Peak RSS (app): %.2f MiB", peakMiB);
            if (p_stats_->processResidentPeakOsBytes > 0) {
                const double peakOsMiB = static_cast<double>(p_stats_->processResidentPeakOsBytes) / (1024.0 * 1024.0);
                ImGui::Text("Peak RSS (OS): %.2f MiB", peakOsMiB);
            }
        } else {
            ImGui::Text("Process memory sampling unavailable on this platform");
        }
        if (p_config_->renderer.selectedRenderer == Config::RendererType::SOFTWARE) {
            const uint64_t swJobsSubmitted = p_stats_->swJobsSubmitted.load(std::memory_order_relaxed);
            const uint64_t swJobsCompleted = p_stats_->swJobsCompleted.load(std::memory_order_relaxed);
            const uint64_t swJobsCancelled = p_stats_->swJobsCancelled.load(std::memory_order_relaxed);
            const uint64_t swJobsDroppedPending = p_stats_->swJobsDroppedPending.load(std::memory_order_relaxed);
            const uint64_t swFramesUploaded = p_stats_->swFramesUploaded.load(std::memory_order_relaxed);
            const uint64_t swFramesDroppedReady = p_stats_->swFramesDroppedReady.load(std::memory_order_relaxed);
            ImGui::SeparatorText("SW Async");
            ImGui::Text("Jobs: submitted=%" PRIu64 " completed=%" PRIu64, swJobsSubmitted, swJobsCompleted);
            ImGui::Text("Jobs: cancelled=%" PRIu64 " dropped(pending)=%" PRIu64, swJobsCancelled,
                        swJobsDroppedPending);
            ImGui::Text("Frames: uploaded=%" PRIu64 " dropped(ready)=%" PRIu64, swFramesUploaded,
                        swFramesDroppedReady);
        }
        if (auto cam = Engine::Get().GetCamera()) {
            ImGui::Text("Camera position: (%.3f, %.3f, %.3f)", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
        }
        assert(p_stats_ != nullptr && "Stats not initialized!");
        ImGui::PlotLines("##frameTimes", frameTimes, IM_ARRAYSIZE(frameTimes), frameIndex, nullptr, 0.0f, 50.0f,
                         ImVec2(0, 80));
        if (ImGui::BeginPopupContextWindow("popupCtx")) {
            if (ImGui::MenuItem("Top-left", nullptr, location == 0))
                location = 0;
            if (ImGui::MenuItem("Top-right", nullptr, location == 1))
                location = 1;
            if (ImGui::MenuItem("Bottom-left", nullptr, location == 2))
                location = 2;
            if (ImGui::MenuItem("Bottom-right", nullptr, location == 3))
                location = 3;
            if (ImGui::MenuItem("Close"))
                p_config_->window.showFPS = false;
            ImGui::EndPopup();
        }
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();
    }
    ImGui::End();
}

void ConfigPanel::DisplayJoysticks() {
    ImGuiIO& io = ImGui::GetIO();
    float moveSpeed = 0.005f;
    float rotateSpeed = 0.1f;
    float deltaTime = 5.0f; // TODO: actually pass

    ImVec2 stickSize(200, 200);
    ImVec2 leftPos(20, io.DisplaySize.y - stickSize.y - 20);
    ImVec2 rightPos(io.DisplaySize.x - stickSize.x - 20, io.DisplaySize.y - stickSize.y - 20);

    auto DrawJoystick = [&](const char* label, const ImVec2& pos, VirtualStickState& state) {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(stickSize);
        ImGui::Begin(label, nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 center = ImVec2(pos.x + stickSize.x * 0.5f, pos.y + stickSize.y * 0.5f);
        float radius = stickSize.x * 0.45f;

        drawList->AddCircleFilled(center, radius, IM_COL32(80, 80, 80, 100));

        bool hovered = ImGui::IsWindowHovered();

        if (!state.active && hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            // Start drag
            state.active = true;
            state.origin = io.MousePos;
        }

        if (state.active) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                // Still dragging, calculate relative movement from origin
                ImVec2 diff = ImVec2(io.MousePos.x - state.origin.x, io.MousePos.y - state.origin.y);
                state.delta.x = diff.x / radius;
                state.delta.y = diff.y / radius;

                float len = sqrtf(state.delta.x * state.delta.x + state.delta.y * state.delta.y);
                if (len > 1.0f) {
                    state.delta.x /= len;
                    state.delta.y /= len;
                }
            } else {
                // Finger lifted
                state.active = false;
                state.delta = ImVec2(0, 0);
            }
        }

        // Draw knob (based on delta)
        ImVec2 knobPos = ImVec2(center.x + state.delta.x * radius * 0.5f, center.y + state.delta.y * radius * 0.5f);
        drawList->AddCircleFilled(knobPos, radius * 0.3f, IM_COL32(200, 200, 200, 180));

        ImGui::End();
    };

    DrawJoystick("MoveStick", leftPos, moveStickState);
    DrawJoystick("RotateStick", rightPos, rotateStickState);

    // Apply camera movement
    if (auto cam = Engine::Get().GetCamera()) {
        if (moveStickState.active) {
            glm::vec3 forward = cam->m_Direction;
            glm::vec3 right = glm::normalize(glm::cross(forward, cam->m_Up));

            cam->m_Position += forward * (-moveStickState.delta.y) * moveSpeed * deltaTime;
            cam->m_Position += right * (moveStickState.delta.x) * moveSpeed * deltaTime;
        }

        if (rotateStickState.active) {
            cam->m_EulerRotation.y += (-rotateStickState.delta.x) * rotateSpeed * deltaTime; // yaw
            cam->m_EulerRotation.x += (-rotateStickState.delta.y) * rotateSpeed * deltaTime; // pitch
        }
    }
}

void ConfigPanel::BeforeFrame() {
    auto const& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x > 0 ? (int)io.DisplaySize.x : 0,
               (int)io.DisplaySize.y > 0 ? (int)io.DisplaySize.y : 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    DisplayGUI();
}

void ConfigPanel::OnDraw() {
    ImGui::Render();
    auto const& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // Multi-viewport support
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        // SDL_Window *backupCurrentWindow = SDL_GL_GetCurrentWindow();
        // SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        // SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
    }
}

void ConfigPanel::Destroy() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ConfigPanel::OpenAndroidFilePicker() {
#ifdef __ANDROID__
    auto* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    auto activity = static_cast<jobject>(SDL_AndroidGetActivity());
    jclass cls = env->GetObjectClass(activity);
    jmethodID mid = env->GetMethodID(cls, "openFilePicker", "()V");
    env->CallVoidMethod(activity, mid);
#endif
}

void ConfigPanel::OpenWebFilePicker() {
#ifdef __EMSCRIPTEN__
    OpenWebFilePicker_JS();
#endif
}
} // namespace RetroRenderer
