#include <imgui.h>
#include <KrisLogger/Logger.h>
#include "DisplaySystem.h"
#include "../Renderer/Buffer.h"
#include "../Engine.h"

namespace RetroRenderer
{
    SDL_Window *DisplaySystem::GetWindow() const
    {
        return m_window_;
    }

    bool DisplaySystem::Init(std::shared_ptr<Camera> camera)
    {
        p_camera_ = camera;
		auto const& p_config = Engine::Get().GetConfig();
		auto const& p_stats = Engine::Get().GetStats();

        int screenWidth = p_config->window.size.x;
        int screenHeight = p_config->window.size.y;

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
#elif defined(__ANDROID_API__)
        const char* glslVersion = "#version 300 es";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#elif defined(__APPLE__)
        // GL 3.2 Core + GLSL 150
        const char* glslVersion = "#version 150";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
        // GL 4.3
        const char* glslVersion = "#version 430";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

        // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif
/*
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        */
        m_window_ = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth,
                                    screenHeight, kWindowFlags);
        if (m_window_ == nullptr)
        {
            LOGE("Unable to create window: %s", SDL_GetError());
            return false;
        }
        m_glContext_ = SDL_GL_CreateContext(m_window_);
        if (!m_glContext_)
        {
            LOGE("Error creating OpenGL context: %s\n", SDL_GetError());
            return false;
        }
        SDL_GL_MakeCurrent(m_window_, m_glContext_);

#ifndef __ANDROID__
        if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
        {
            LOGE("Error initializing GLAD\n");
            return false;
        }
#endif

        LOGI("OpenGL loaded");
        LOGI("OpenGL Vendor:    %s", glGetString(GL_VENDOR));
        LOGI("OpenGL Renderer:  %s", glGetString(GL_RENDERER));
        LOGI("OpenGL Version:   %s", glGetString(GL_VERSION));
        LOGI("GLSL Version:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        LOGI("OpenGL extensions:     %s", glGetString(GL_EXTENSIONS));

        int contextFlags;
        /*
        glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
        if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            LOGI("Debug context is active!");
        }
        else {
            LOGW("Debug context not available!");
        }
        */
        glViewport(0, 0, screenWidth, screenHeight);
        // glEnable(GL_DEPTH_TEST);

        SDL_GL_SetSwapInterval(p_config->window.enableVsync ? 1 : 0);

        m_configPanel_ = std::make_unique<ConfigPanel>();
        m_configPanel_->Init(m_window_, m_glContext_, p_config, p_camera_, glslVersion, p_stats);
        return true;
    }

    void DisplaySystem::BeforeFrame()
    {
        ResetGlContext();
        m_configPanel_->BeforeFrame();
    }

    void DisplaySystem::DrawFrame()
    {
        m_configPanel_->DisplayRenderedImage();
        m_configPanel_->OnDraw();
    }

    void DisplaySystem::DrawFrame(GLuint p_framebufferTexture)
    {
        ResetGlContext();
        m_configPanel_->DisplayRenderedImage(p_framebufferTexture);
        m_configPanel_->OnDraw();
    }

    void DisplaySystem::SwapBuffers()
    {
        SDL_GL_MakeCurrent(m_window_, m_glContext_);
        SDL_GL_SwapWindow(m_window_);
    }

    void DisplaySystem::Destroy()
    {
        SDL_GL_DeleteContext(m_glContext_);
        SDL_DestroyWindow(m_window_);
        SDL_Quit();
    }

    void DisplaySystem::ResetGlContext()
    {
        SDL_GL_MakeCurrent(m_window_, m_glContext_);
    }
}