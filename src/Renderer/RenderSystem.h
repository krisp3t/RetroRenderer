#pragma once

#include <memory>
#include "../Window/DisplaySystem.h"
#include "../Base/Event.h"
#include "../Scene/Scene.h"
#include "Software/SWRenderer.h"

namespace RetroRenderer
{

class RenderSystem
{
public:
    RenderSystem() = default;
    ~RenderSystem() = default;

    bool Init(DisplaySystem& displaySystem);
    void BeforeFrame(Uint32 clearColor);
    std::queue<Model*>& BuildRenderQueue(Scene &scene, const Camera &camera);
    void Render(std::queue<Model *>& renderQueue);
    void TestFill();
    void Destroy();
    void OnLoadScene(const SceneLoadEvent& e);
private:
    DisplaySystem* pDisplaySystem = nullptr;
	std::unique_ptr<Scene> pScene = nullptr;
    std::unique_ptr<SWRenderer> pSWRenderer = nullptr;

};

}



