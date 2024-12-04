#include <imgui.h>
#include <glad/glad.h>
#include "DisplaySystem.h"
#include "../Base/Logger.h"
#include "../Renderer/Buffer.h"

namespace RetroRenderer
{
    bool DisplaySystem::Init(std::shared_ptr<Config> config, std::weak_ptr<Camera> camera)
    {
        p_Config = config;
        p_Camera = camera;

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            LOGE("Unable to initialize SDL: %s\n", SDL_GetError());
            return false;
        }
    #if defined(IMGUI_IMPL_OPENGL_ES2)
		// GL ES 2.0 + GLSL 100
		const char* glsl_version = "#version 100";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #elif defined(__APPLE__)
		// GL 3.2 Core + GLSL 150
		const char* glsl_version = "#version 150";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    #else
		// GL 3.0 + GLSL 130
		const char* glsl_version = "#version 130";
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
        m_Window = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_ScreenWidth, m_ScreenHeight, kWindowFlags);
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
        // TODO: init glad?
        /*
		if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
			LOGE("Failed to initialize GLAD\n");
			return false;
		}
        */

		// SDL_GL_SetSwapInterval(1); // Enable vsync
        m_ConfigPanel = std::make_unique<ConfigPanel>(m_Window, m_SDLRenderer, p_Config, p_Camera);
        return true;
    }

    void DisplaySystem::BeforeFrame(Uint32 c) {
        //SDL_SetRenderDrawColor(m_SDLRenderer, c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, (c >> 24) & 0xFF);
        //SDL_RenderClear(m_SDLRenderer);
        //glViewport(0, 0, 
        glClearColor(0, 0, 0, 0);
        
        //m_ConfigPanel.get()->BeforeFrame(m_SDLRenderer);
    }

    void DisplaySystem::DrawFrame()
    {
        m_ConfigPanel.get()->OnDraw();
    }
    void DisplaySystem::DrawFrame(const Buffer<Uint32> &buffer)
    {
        assert(buffer.width == m_ScreenWidth && buffer.height == m_ScreenHeight && "Buffer size does not match window size");
        assert(buffer.data != nullptr && "Buffer data is null");
        assert(m_ScreenTexture != nullptr && "Screen texture is null");

		// TODO: Replace with OpenGL texture

        /*
		SDL_SetRenderTarget(m_SDLRenderer, m_ScreenTexture); // Write framebuffer to texture
        const Uint32* src = buffer.data;
        SDL_UpdateTexture(m_ScreenTexture, nullptr, src, static_cast<int>(buffer.width * sizeof(Uint32)));
        SDL_RenderCopy(m_SDLRenderer, m_ScreenTexture, nullptr, nullptr);
		SDL_SetRenderTarget(m_SDLRenderer, nullptr); // SDL renderer back to default target (screen)
        */

        m_ConfigPanel.get()->OnDraw();
    }

    void DisplaySystem::SwapBuffers()
    {
        SDL_RenderPresent(m_SDLRenderer);
    }

    int DisplaySystem::GetWidth() const
    {
        return m_ScreenWidth;
    }
    int DisplaySystem::GetHeight() const
    {
        return m_ScreenHeight;
    }

    void DisplaySystem::Destroy()
    {
        SDL_GL_DeleteContext(m_glContext);
        SDL_DestroyWindow(m_Window);
        SDL_Quit();
    }
}