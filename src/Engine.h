#pragma once
#include "Window/DisplaySystem.h"
#include "Renderer/RenderSystem.h"
#include "Window/InputSystem.h"

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
    DisplaySystem m_DisplaySystem;
    RenderSystem m_RenderSystem;
    InputSystem m_InputSystem;
};

}
