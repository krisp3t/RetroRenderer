/***
 * Program initialization and shutdown.
 * @param argc
 * @param args
 * @return
 */

#include "Engine.h"
#include <KrisLogger/Logger.h>
#include <android_native_app_glue.h>

int main(int argc, char *args[])
{
    auto &retro = RetroRenderer::Engine::Get();
    if (!retro.Init())
    {
        LOGE("Failed to initialize RetroRenderer");
        return 1;
    }
    retro.Run();
    retro.Destroy();
    return 0;
}

void android_main(struct android_app *app)
{
    // Your game/application logic here
}