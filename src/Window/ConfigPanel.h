#pragma once
#include <vector>
#include "../Base/Config.h"
#include "../Base/Stats.h"
#include "../Scene/Camera.h"
#include "../include/kris_glheaders.h"


namespace RetroRenderer
{
struct VirtualStickState {
    bool active = false;
    ImVec2 origin;   // where finger first touched
    ImVec2 delta;    // normalized [-1, 1]
    SDL_FingerID fingerId = -1; // not needed for mouse, useful for multi-touch
};
class ConfigPanel
{
public:
    ConfigPanel() = default;
    ~ConfigPanel();
    bool Init(SDL_Window *window,
              SDL_GLContext glContext,
              std::shared_ptr<Config> config,
              const char* glslVersion,
              std::shared_ptr<Stats> stats
              );
    void Destroy();
    void BeforeFrame();
	void DisplayRenderedImage();
	void DisplayRenderedImage(GLuint p_framebufferTexture);
    void OnDraw();
    void OnDraw(GLuint framebufferTexture);
private:
	SDL_Window* p_Window_ = nullptr;
    std::shared_ptr<Config> p_config_ = nullptr;
    std::shared_ptr<Stats> p_stats_ = nullptr;
    std::vector<uint8_t> m_fontData_ = {}; // keep font bytes alive for the lifetime of imgui
	bool m_isDragging_ = false;
    bool m_isFileDialogOpen_ = false;

    void StyleColorsEnemymouse();
    void DisplayGUI();
    void DisplayMetricsOverlay();
    void DisplayCameraSettings();
    void DisplayCullSettings();
    void DisplayRendererSettings();
    void DisplayRasterizerSettings();
    void DisplayEnvironmentSettings();
    void DisplayMainMenu();
    void DisplaySceneGraph();
    void DisplayMaterialWindow();
    void DisplayInspectorWindow();
    void DisplayPipelineWindow();
    void DisplayConfigWindow(Config& config);
    void DisplayControlsOverlay();
    void DisplayExamplesDialog();
    void DisplayWindowSettings();
    void DisplayJoysticks();
    void OpenWebFilePicker();
    void OpenAndroidFilePicker();

    const char* k_supportedModels = ".obj,.gltf,.glb,.fbx,.usd";
    VirtualStickState moveStickState;
    VirtualStickState rotateStickState;
};

}





