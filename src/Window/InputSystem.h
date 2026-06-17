#pragma once
#include "../Base/Config.h"
#include "../Base/Event.h"
#include "../Base/InputActions.h"
#include <SDL.h>
#include <functional>
#include <memory>

namespace RetroRenderer {

class InputSystem {
  public:
    InputSystem() = default;
    ~InputSystem() = default;

    using EventEnqueueFn = std::function<void(std::unique_ptr<Event>)>;

    bool Init(std::shared_ptr<Config> config, EventEnqueueFn enqueueEvent);

    [[nodiscard]] InputActionMask HandleInput();

    void Destroy();

  private:
    void HandleKeyDown(SDL_Keycode key, Config& config);
    void PollContinuousInput(bool keyboardCaptured);
    void HandleMouseMotion(const SDL_MouseMotionEvent& event);

    std::shared_ptr<Config> p_Config_ = nullptr;
    EventEnqueueFn m_EnqueueEvent_;
    InputActionMask m_inputState_ = 0;
    // bool m_isDragging_ = false;
};

} // namespace RetroRenderer
