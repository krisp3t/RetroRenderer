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

    bool Init();

    [[nodiscard]] InputActionMask HandleInput();

    void Destroy();
private:
    void HandleKeyDown(SDL_Keycode key, Config& config);
    void HandleMouseMotion(const SDL_MouseMotionEvent& event);

    InputActionMask m_inputState_ = 0;
    // bool m_isDragging_ = false;
};

}

