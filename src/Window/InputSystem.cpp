#ifdef __EMSCRIPTEN__
#include "../native/emscripten/imgui_impl_sdl2.h"
#else
#include <imgui_impl_sdl2.h>
#endif
#include "../Engine.h"
#include "InputSystem.h"
#include <KrisLogger/Logger.h>

namespace RetroRenderer {
namespace {
bool ShouldBlockKeyboardGameInput() {
    if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
        return false;
    }

    const ImGuiIO& io = ImGui::GetIO();
    return io.WantTextInput || ImGui::IsAnyItemActive();
}
} // namespace

bool InputSystem::Init() {
    return true;
}

InputActionMask InputSystem::HandleInput() {
    auto const& p_config = Engine::Get().GetConfig();
    m_inputState_ = static_cast<InputActionMask>(0);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        ImGuiIO& io = ImGui::GetIO();
        switch (event.type) {
        case SDL_QUIT:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::QUIT);
            return m_inputState_;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                m_inputState_ |= static_cast<InputActionMask>(InputAction::QUIT);
                return m_inputState_;
            }
            if (ShouldBlockKeyboardGameInput()) {
                break;
            }
            HandleKeyDown(event.key.keysym.sym, *p_config);
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                auto e = std::make_unique<WindowResizeEvent>(glm::ivec2{event.window.data1, event.window.data2});
                Engine::Get().EnqueueEvent(std::move(e));
            }
            break;
        }
    }
    PollContinuousInput(ShouldBlockKeyboardGameInput());
    return m_inputState_;
}

void InputSystem::HandleKeyDown(SDL_Keycode key, Config& config) {
    switch (key) {
    case SDLK_h:
        config.window.showConfigPanel = !config.window.showConfigPanel;
        // m_inputState_ |= static_cast<InputActionMask>(InputAction::TOGGLE_CONFIG_PANEL);
        break;
    case SDLK_1:
        m_inputState_ |= static_cast<InputActionMask>(InputAction::TOGGLE_WIREFRAME);
        break;
    }
}

void InputSystem::PollContinuousInput(bool keyboardCaptured) {
    if (keyboardCaptured) {
        return;
    }

    const Uint8* keyState = SDL_GetKeyboardState(nullptr);
    if (!keyState) {
        return;
    }

    if (keyState[SDL_SCANCODE_W]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_FORWARD);
    }
    if (keyState[SDL_SCANCODE_S]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_BACKWARD);
    }
    if (keyState[SDL_SCANCODE_A]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_LEFT);
    }
    if (keyState[SDL_SCANCODE_D]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_RIGHT);
    }
    if (keyState[SDL_SCANCODE_SPACE]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_UP);
    }
    if (keyState[SDL_SCANCODE_LSHIFT]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_DOWN);
    }
    if (keyState[SDL_SCANCODE_LEFT]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::ROTATE_LEFT);
    }
    if (keyState[SDL_SCANCODE_RIGHT]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::ROTATE_RIGHT);
    }
    if (keyState[SDL_SCANCODE_UP]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::ROTATE_UP);
    }
    if (keyState[SDL_SCANCODE_DOWN]) {
        m_inputState_ |= static_cast<InputActionMask>(InputAction::ROTATE_DOWN);
    }
}

void InputSystem::HandleMouseMotion(const SDL_MouseMotionEvent& event) {
    // Currently handled in imgui
    /*
    if (!m_isDragging_)
    {
        return;
    }

    // Relative motions
    int dx = event.xrel;
    int dy = event.yrel;
    */
}

void InputSystem::Destroy() {
}

} // namespace RetroRenderer
