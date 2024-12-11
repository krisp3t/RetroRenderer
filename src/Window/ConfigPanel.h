#pragma once

#include <GLAD/glad.h>
#include "../Base/Config.h"
#include "../Base/Stats.h"
#include "../Scene/Camera.h"

namespace RetroRenderer
{

class ConfigPanel
{
public:
    ConfigPanel(SDL_Window *window,
                SDL_GLContext glContext,
                std::shared_ptr<Config> config,
                std::shared_ptr<Camera> camera,
                const char* glslVersion,
                std::shared_ptr<Stats> stats
                );
    ~ConfigPanel();
    bool Init(SDL_Window *window,
              SDL_GLContext glContext,
              std::shared_ptr<Config> config,
              std::shared_ptr<Camera> camera,
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
	SDL_Window* m_Window = nullptr;
    std::shared_ptr<Config> p_Config = nullptr;
    std::shared_ptr<Camera> p_Camera;
    std::shared_ptr<Stats> p_Stats = nullptr;
	bool m_isDragging = false;

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
    void DisplayPipelineWindow();
    void DisplayConfigWindow(Config& config);
    void DisplayControlsOverlay();
};

}





