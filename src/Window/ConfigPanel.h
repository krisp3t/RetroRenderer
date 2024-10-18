#pragma once

#include "../Base/Config.h"

namespace RetroRenderer
{

class ConfigPanel
{
public:
    ConfigPanel(SDL_Window *window, SDL_Renderer *renderer, std::shared_ptr<Config> config);
    ~ConfigPanel();
    bool Init(SDL_Window *window, SDL_Renderer *renderer, std::shared_ptr<Config> config);
    void Destroy();
    void BeforeFrame(SDL_Renderer *renderer);
    void OnDraw();
private:
    std::shared_ptr<Config> p_Config = nullptr;
    void DisplayConfig(Config &config);
    void DisplayGUI();
    void DisplayMetricsOverlay();
    void DisplayCameraSettings();
    void DisplayRendererSettings();
    void DisplayEnvironmentSettings();
    void DisplayMainMenu();
    void DisplayControlsOverlay();
};

}





