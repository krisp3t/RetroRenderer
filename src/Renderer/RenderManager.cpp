#include <cassert>
#include "RenderManager.h"

namespace RetroRenderer
{
    bool RenderManager::Init(DisplayManager& displayManager)
    {
        this->pDisplayManager = &displayManager;
        return true;
    }

    void RenderManager::Render()
    {
        assert(pDisplayManager != nullptr && "DisplayManager is null");

        pDisplayManager->BeforeFrame();
        pDisplayManager->DrawFrame();
        pDisplayManager->SwapBuffers();
    }

    void RenderManager::Destroy()
    {
    }

}