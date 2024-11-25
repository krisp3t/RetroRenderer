#include <cassert>
#include "RenderSystem.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
    bool RenderSystem::Init(DisplaySystem& displaySystem)
    {
        pDisplaySystem = &displaySystem;
        pSWRenderer = std::make_unique<SWRenderer>();
        if (!pSWRenderer->Init(pDisplaySystem->GetWidth(), pDisplaySystem->GetHeight()))
        {
            LOGE("Failed to initialize SWRenderer");
            return false;
        }

        LOGD("SWRenderer initialized");
        return true;
    }

    void RenderSystem::Render()
    {
        assert(pDisplaySystem != nullptr && "DisplaySystem is null");

        pDisplaySystem->BeforeFrame();

		if (pScene == nullptr)
		{
            pDisplaySystem->DrawFrame();
            pDisplaySystem->SwapBuffers();
            return;
		}

        auto &selectedRenderer = pSWRenderer; // TODO: get from config
        assert(selectedRenderer != nullptr && "Selected renderer is null");
        selectedRenderer->DrawFrame(*pScene);
        const auto &fb = selectedRenderer->GetRenderTarget();

        pDisplaySystem->DrawFrame(fb);

        pDisplaySystem->SwapBuffers();
    }

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnLoadScene(const SceneLoadEvent& e)
    {

    }

}