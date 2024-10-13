#pragma once

namespace RetroRenderer
{

class ConfigPanel
{
public:
    ConfigPanel(SDL_Window *window, SDL_Renderer *renderer);
    ~ConfigPanel();
    bool Init(SDL_Window *window, SDL_Renderer *renderer);
    void Destroy();
    void BeforeFrame(SDL_Renderer *renderer);
    void OnDraw();
private:
    void DisplayGUI();
    void DisplayMetricsOverlay();
    void DisplayRendererSettings();
    void DisplayMainMenu();
    void DisplayControlsOverlay();
};

}





