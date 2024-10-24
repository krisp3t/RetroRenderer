#include <cassert>
#include "RenderSystem.h"
#include "../Base/Event.h"
#include "../Base/Logger.h"

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

    void RenderSystem::OnLoadScene(const SceneLoadEvent& e)
    {
		LOGD("Attempting to load scene from path: %s", e.scenePath.c_str());
    }

}