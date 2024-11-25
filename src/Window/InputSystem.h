#pragma once
#include "../Window/DisplaySystem.h"
#include "../Base/InputActions.h"

namespace RetroRenderer
{

class InputSystem
{
public:
    InputSystem() = default;
    ~InputSystem() = default;

    bool Init(std::shared_ptr<Config> config);
    [[nodiscard]] InputActionMask HandleInput();
    void Destroy();
private:
    std::shared_ptr<Config> p_Config = nullptr;
    void HandleKeyDown(SDL_Keycode key);
    InputActionMask m_InputState = 0;
};

}

