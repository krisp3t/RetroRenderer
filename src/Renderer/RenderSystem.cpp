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

	void RenderSystem::BeforeFrame(Uint32 clearColor)
	{
        // TODO: utility function?
		Uint32 a = (clearColor & 0xFF000000) >> 24;
		Uint32 b = (clearColor & 0x00FF0000) >> 16;
		Uint32 g = (clearColor & 0x0000FF00) >> 8;
		Uint32 r = (clearColor & 0x000000FF);
        Uint32 argbColor = (a << 24) | (r << 16) | (g << 8) | b;
		pSWRenderer->GetRenderTarget().Clear(argbColor);
	}

    std::queue<Model*>& RenderSystem::BuildRenderQueue(Scene& scene, const Camera& camera)
    {
        auto &activeRenderer = pSWRenderer; // TODO: get from config
        activeRenderer->SetActiveCamera(camera);
        return scene.GetVisibleModels();
    }

    void RenderSystem::Render(std::queue<Model *>& renderQueue)
    {
        auto &activeRenderer = pSWRenderer; // TODO: get from config
        assert(activeRenderer != nullptr && "Active renderer is null");

        while (!renderQueue.empty())
        {
			//LOGD("Render queue size: %d", renderQueue.size());
            auto model = renderQueue.front();
            activeRenderer->DrawTriangularMesh(*model->GetMesh());
            renderQueue.pop();
        }

        const auto &fb = activeRenderer->GetRenderTarget();
        pDisplaySystem->DrawFrame(fb);
    }

    void RenderSystem::TestFill()
    {
		auto& activeRenderer = pSWRenderer; // TODO: get from config
		assert(activeRenderer != nullptr && "Active renderer is null");

		Buffer<Uint32>& fb = activeRenderer->GetRenderTarget();
		fb.Clear(static_cast<Uint32>(rand()));

		pDisplaySystem->DrawFrame(fb);
	}

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnLoadScene(const SceneLoadEvent& e)
    {

    }

}