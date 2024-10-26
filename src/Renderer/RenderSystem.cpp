#include <cassert>
#include "RenderSystem.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
    bool RenderSystem::Init(DisplaySystem& displaySystem)
    {
        pDisplaySystem = &displaySystem;
        pSWRenderer = std::make_unique<SWRenderer>();
        // TODO: check if renderer init succeeded
        return true;
    }

    void RenderSystem::Render()
    {
        assert(pDisplaySystem != nullptr && "DisplaySystem is null");

        pDisplaySystem->BeforeFrame();
        pDisplaySystem->DrawFrame();
        pDisplaySystem->SwapBuffers();

		if (pScene == nullptr)
		{
            return;
		}

        auto &selectedRenderer = pSWRenderer; // TODO: get from config
        assert(selectedRenderer != nullptr && "Selected renderer is null");
        selectedRenderer->DrawFrame(*pScene);
    }

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnLoadScene(const SceneLoadEvent& e)
    {
		LOGD("Attempting to load scene from path: %s", e.scenePath.c_str());
		pScene = std::make_unique<Scene>(e.scenePath);
    }

}