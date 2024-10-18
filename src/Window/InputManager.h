#pragma once
#include "../Window/DisplayManager.h"

namespace RetroRenderer
{

class InputManager
{
public:
    InputManager() = default;
    ~InputManager() = default;

    bool Init(std::shared_ptr<Config> config);
    void HandleInput(bool &isRunning);
    void Update();
    void Destroy();
private:
    std::shared_ptr<Config> p_Config = nullptr;
    void HandleKeyDown(SDL_Keycode key);
};

}

