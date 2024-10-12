#pragma once
#include "../Window/DisplayManager.h"

namespace RetroRenderer
{

class InputManager
{
public:
    InputManager() = default;
    ~InputManager() = default;

    bool Init(DisplayManager& displayManager);
    void HandleInput(bool &isRunning);
    void Update();
    void Destroy();

};

}

