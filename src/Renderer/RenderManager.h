#pragma once
#include "../Window/DisplayManager.h"

namespace RetroRenderer
{

class RenderManager
{
public:
    RenderManager() = default;
    ~RenderManager() = default;

    bool Init(DisplayManager& displayManager);
    void Render();
    void Destroy();
private:
    DisplayManager* pDisplayManager = nullptr;
};

}



