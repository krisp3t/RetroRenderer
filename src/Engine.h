#pragma once
#include "Window/Window.h"

namespace RetroRenderer
{

class Engine
{
public:
    Engine() = default;
    ~Engine() = default;

    bool Init();
    void Destroy();
private:
    Window mWindow;
};

}
