#pragma once
#include "../Window/DisplaySystem.h"

namespace RetroRenderer
{

class RenderSystem
{
public:
    RenderSystem() = default;
    ~RenderSystem() = default;

    bool Init(DisplaySystem& displaySysteem);
    void Render();
    void Destroy();
private:
    DisplaySystem* pDisplaySystem = nullptr;
};

}



