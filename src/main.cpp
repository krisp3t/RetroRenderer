/***
 * Program initialization and shutdown.
 * @param argc
 * @param args
 * @return
 */

#include "Engine.h"
#include "Base/Logger.h"

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