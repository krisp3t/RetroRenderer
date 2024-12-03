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

    void RenderSystem::BuildRenderQueue(const Scene& scene, const Camera& camera)
    {
        auto &activeRenderer = pSWRenderer; // TODO: get from config
        activeRenderer->SetActiveCamera(camera);
        scene.GetVisibleModels();
    }

    void RenderSystem::Render()
    {
        auto &activeRenderer = pSWRenderer; // TODO: get from config
        assert(activeRenderer != nullptr && "Active renderer is null");

        //activeRenderer->DrawFrame(*pScene);
        const auto &fb = activeRenderer->GetRenderTarget();

        //pDisplaySystem->DrawFrame(fb);
    }

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnLoadScene(const SceneLoadEvent& e)
    {

    }

}