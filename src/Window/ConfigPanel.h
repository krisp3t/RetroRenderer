#pragma once

#include "../Base/Config.h"
#include "../Scene/Camera.h"

namespace RetroRenderer
{

class ConfigPanel
{
public:
    ConfigPanel(SDL_Window *window,
        SDL_GLContext glContext,
                std::shared_ptr<Config> config,
                std::weak_ptr<Camera> camera
                );
    ~ConfigPanel();
    bool Init(SDL_Window *window,
        SDL_GLContext glContext,
              std::shared_ptr<Config> config,
              std::weak_ptr<Camera> camera
              );
    void Destroy();
    void BeforeFrame(SDL_Renderer *renderer);
    void OnDraw();
private:
    std::shared_ptr<Config> p_Config = nullptr;
    std::weak_ptr<Camera> p_Camera;

    void StyleColorsEnemymouse();
    void DisplayGUI();
    void DisplayMetricsOverlay();
    void DisplayCameraSettings();
    void DisplayRendererSettings();
    void DisplayEnvironmentSettings();
    void DisplayMainMenu();
    void DisplayPipelineWindow();
    void DisplayConfigWindow(Config& config);
    void DisplayControlsOverlay();
};

}





