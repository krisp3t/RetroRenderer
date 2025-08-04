#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include "ConfigPanel.h"
#include "../Renderer/Buffer.h"
#include "../Scene/Camera.h"
#include "../Base/Stats.h"
#include "../Base/Color.h"
#ifdef __ANDROID__
#include <GLES3/gl3.h> // For OpenGL ES 3.0
#else
#include <glad/glad.h>
#endif

namespace RetroRenderer
{
    class DisplaySystem
    {
    public:
        static constexpr SDL_WindowFlags kWindowFlags =
            static_cast<const SDL_WindowFlags>(
                SDL_WINDOW_OPENGL |
                SDL_WINDOW_MAXIMIZED |
                SDL_WINDOW_RESIZABLE |
                SDL_WINDOW_ALLOW_HIGHDPI);
        static constexpr char kWindowTitle[] = "RetroRenderer";

        DisplaySystem() = default;

        ~DisplaySystem() = default;

        SDL_Window *GetWindow() const;

        bool Init(std::shared_ptr<Camera> camera);

        void Destroy();

        void SwapBuffers();

        void BeforeFrame();

        void DrawFrame();

        void DrawFrame(GLuint framebufferTexture);

        void ResetGlContext();
    private:
        SDL_Window *m_window_ = nullptr;
        SDL_GLContext m_glContext_ = nullptr;
        std::unique_ptr<ConfigPanel> m_configPanel_ = nullptr;
        std::shared_ptr<Camera> p_camera_ = nullptr;
    };

}
