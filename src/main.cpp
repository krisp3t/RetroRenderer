#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <KrisLogger/Logger.h>
#include "Engine.h"

int SDL_main(int argc, char** argv) {
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

#ifdef __ANDROID__
extern "C" void android_main(struct android_app* app)
{
    SDL_main(0, nullptr);
}
#else
int main(int argc, char** argv)
{
    return SDL_main(argc, argv);
}
#endif
