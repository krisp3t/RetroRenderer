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

        pDisplayManager->Clear();
        pDisplayManager->DrawConfigPanel();
        pDisplayManager->SwapBuffers();
    }

    void RenderManager::Destroy()
    {
    }

}