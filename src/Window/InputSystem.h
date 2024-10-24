#pragma once
#include "../Window/DisplaySystem.h"

namespace RetroRenderer
{

class InputSystem
{
public:
    InputSystem() = default;
    ~InputSystem() = default;

    bool Init(std::shared_ptr<Config> config);
    void HandleInput(bool &isRunning);
    void Update();
    void Destroy();
private:
    std::shared_ptr<Config> p_Config = nullptr;
    void HandleKeyDown(SDL_Keycode key);
};

}

