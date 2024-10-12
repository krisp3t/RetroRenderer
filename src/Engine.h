#pragma once
#include "Window/DisplayManager.h"
#include "Renderer/RenderManager.h"

namespace RetroRenderer
{

class Engine
{
public:
    Engine() = default;
    ~Engine() = default;

    bool Init();
    void Run();
    void Destroy();
private:
    DisplayManager m_DisplayManager;
    RenderManager m_RenderManager;
};

}
