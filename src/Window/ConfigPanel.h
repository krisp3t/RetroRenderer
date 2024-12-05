#pragma once

#include <GLAD/glad.h>
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
                std::weak_ptr<Camera> camera,
                const char* glslVersion
                );
    ~ConfigPanel();
    bool Init(SDL_Window *window,
              SDL_GLContext glContext,
              std::shared_ptr<Config> config,
              std::weak_ptr<Camera> camera,
              const char* glslVersion
              );
    void Destroy();
    void BeforeFrame();
	void DisplayRenderedImage();
	void DisplayRenderedImage(GLuint p_framebufferTexture);
    void OnDraw();
    void OnDraw(GLuint framebufferTexture);
private:
    std::shared_ptr<Config> p_Config = nullptr;
    std::weak_ptr<Camera> p_Camera;

    void StyleColorsEnemymouse();
    void DisplayGUI();
    void DisplayMetricsOverlay();
    void DisplayCameraSettings();
    void DisplayRendererSettings();
    void DisplayRasterizerSettings();
    void DisplayEnvironmentSettings();
    void DisplayMainMenu();
    void DisplayPipelineWindow();
    void DisplayConfigWindow(Config& config);
    void DisplayControlsOverlay();
};

}





