/***
 * Program initialization and shutdown.
 * @param argc
 * @param args
 * @return
 */

#include <KrisLogger/Logger.h>
#include "Engine.h"
#ifdef __ANDROID__
#include <SDL.h>
#endif

#ifdef __ANDROID__
int SDL_main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        exit(1); // Exit if initialization fails.
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window *window = SDL_CreateWindow("RetroRenderer",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          0, 0,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!window)
    {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        SDL_Log("OpenGL context creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }


    bool running = true;
    SDL_Event event;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
        SDL_Delay(1000 / 60);
    }

    // Cleanup before quitting.
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    // Instead of returning normally, call exit() so that the native thread ends cleanly.
    exit(0);
}

extern "C" void android_main(struct android_app* app)
{
    SDL_main(0, nullptr);
}

#else
int main(int argc, char* args[])
{
	auto& retro = RetroRenderer::Engine::Get();
	if (!retro.Init())
	{
		LOGE("Failed to initialize RetroRenderer");
		return 1;
	}
	retro.Run();
	retro.Destroy();
	return 0;
}
#endif