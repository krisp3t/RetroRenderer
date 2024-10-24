#include <cassert>
#include "RenderSystem.h"

namespace RetroRenderer
{
    bool RenderSystem::Init(DisplaySystem& displaySystem)
    {
        this->pDisplaySystem = &displaySystem;
        return true;
    }

    void RenderSystem::Render()
    {
        assert(pDisplaySystem != nullptr && "DisplaySystem is null");

        pDisplaySystem->BeforeFrame();
        pDisplaySystem->DrawFrame();
        pDisplaySystem->SwapBuffers();
    }

    void RenderSystem::Destroy()
    {
    }

}