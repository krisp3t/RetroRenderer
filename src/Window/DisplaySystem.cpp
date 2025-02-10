#include <imgui.h>
#include <GLES3/gl3.h>
//#include <glad/glad.h>
#include <KrisLogger/Logger.h>
#include "DisplaySystem.h"
#include "../Renderer/Buffer.h"

namespace RetroRenderer
{
    SDL_Window *DisplaySystem::GetWindow() const
    {
        return m_Window;
    }

    bool
    DisplaySystem::Init(std::shared_ptr<Config> config, std::shared_ptr<Camera> camera, std::shared_ptr<Stats> stats)
    {
        p_Config = config;
        p_Camera = camera;

        int screenWidth = p_Config->window.size.x;
        int screenHeight = p_Config->window.size.y;

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            LOGE("Unable to initialize SDL: %s\n", SDL_GetError());
            return false;
        }
        SDL_GL_LoadLibrary(nullptr); // Load default OpenGL library
        // TODO: only enable GLAD extensions which are actually used
#if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char* glslVersion = "#version 100";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
        // GL 3.2 Core + GLSL 150
        const char* glslVersion = "#version 150";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
        // GL 3.0 + GLSL 130
        const char *glslVersion = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

        // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        m_Window = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth,
                                    screenHeight, kWindowFlags);
        if (m_Window == nullptr)
        {
            LOGE("Unable to create window: %s", SDL_GetError());
            return false;
        }
        m_glContext = SDL_GL_CreateContext(m_Window);
        if (!m_glContext)
        {
            LOGE("Error creating OpenGL context: %s\n", SDL_GetError());
            return false;
        }
        SDL_GL_MakeCurrent(m_Window, m_glContext);

        /*
        if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
        {
            LOGE("Error initializing GLAD\n");
            return false;
        }*/
        LOGI("OpenGL loaded");
        LOGI("OpenGL Vendor:    %s", glGetString(GL_VENDOR));
        LOGI("OpenGL Renderer:  %s", glGetString(GL_RENDERER));
        LOGI("OpenGL Version:   %s", glGetString(GL_VERSION));
        LOGI("GLSL Version:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        //LOGI("OpenGL extensions:     %s", glGetString(GL_EXTENSIONS));

        glViewport(0, 0, screenWidth, screenHeight);
        // glEnable(GL_DEPTH_TEST);

        SDL_GL_SetSwapInterval(p_Config->window.enableVsync ? 1 : 0);

        m_ConfigPanel = std::make_unique<ConfigPanel>(m_Window, m_glContext, p_Config, p_Camera, glslVersion, stats);
        return true;
    }

    void DisplaySystem::BeforeFrame()
    {
        ResetGlContext();
        glViewport(0, 0, p_Config->window.size.x, p_Config->window.size.y);
        // color is cleared in imgui loop
        m_ConfigPanel.get()->BeforeFrame();
    }

    void DisplaySystem::DrawFrame()
    {
        m_ConfigPanel->DisplayRenderedImage();
        m_ConfigPanel->OnDraw();
    }

    void DisplaySystem::DrawFrame(GLuint p_framebufferTexture)
    {
        ResetGlContext();
        m_ConfigPanel->DisplayRenderedImage(p_framebufferTexture);
        m_ConfigPanel->OnDraw();
    }

    void DisplaySystem::SwapBuffers()
    {
        SDL_GL_MakeCurrent(m_Window, m_glContext);
        SDL_GL_SwapWindow(m_Window);
    }

    void DisplaySystem::Destroy()
    {
        SDL_GL_DeleteContext(m_glContext);
        SDL_DestroyWindow(m_Window);
        SDL_Quit();
    }

    void DisplaySystem::ResetGlContext()
    {
        SDL_GL_MakeCurrent(m_Window, m_glContext);
    }
}