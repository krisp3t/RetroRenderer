#pragma once

#include <memory>
#include "../Window/DisplaySystem.h"
#include "../Base/Event.h"
#include "Scene.h"
#include "Software/SWRenderer.h"

namespace RetroRenderer
{

class RenderSystem
{
public:
    RenderSystem() = default;
    ~RenderSystem() = default;

    bool Init(DisplaySystem& displaySystem);
    void Render();
    void Destroy();
    void OnLoadScene(const SceneLoadEvent& e);
private:
    DisplaySystem* pDisplaySystem = nullptr;
	std::unique_ptr<Scene> pScene = nullptr;
    std::unique_ptr<SWRenderer> pSWRenderer = nullptr;
};

}


