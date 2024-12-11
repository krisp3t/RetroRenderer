#include <imgui_impl_sdl2.h>
#include "InputSystem.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
    bool InputSystem::Init(std::shared_ptr<Config> config)
    {
        p_Config = config;
        return true;
    }

    InputActionMask InputSystem::HandleInput()
    {
        m_InputState = static_cast<InputActionMask>(0);
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
			ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
                case SDL_QUIT:
                    m_InputState |= static_cast<InputActionMask>(InputAction::QUIT);
                    return m_InputState;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        m_InputState |= static_cast<InputActionMask>(InputAction::QUIT);
                        return m_InputState;
                    }
                    HandleKeyDown(event.key.keysym.sym);
                    break;
            }
        }
        return m_InputState;
    }

    void InputSystem::HandleKeyDown(SDL_Keycode key)
    {
        switch (key)
        {
        case SDLK_w:
            m_InputState |= static_cast<InputActionMask>(InputAction::MOVE_FORWARD);
            break;
        case SDLK_s:
            m_InputState |= static_cast<InputActionMask>(InputAction::MOVE_BACKWARD);
            break;
        case SDLK_a:
            m_InputState |= static_cast<InputActionMask>(InputAction::MOVE_LEFT);
            break;
        case SDLK_d:
            m_InputState |= static_cast<InputActionMask>(InputAction::MOVE_RIGHT);
            break;
        case SDLK_LSHIFT:
            m_InputState |= static_cast<InputActionMask>(InputAction::MOVE_UP);
            break;
        case SDLK_LCTRL:
            m_InputState |= static_cast<InputActionMask>(InputAction::MOVE_DOWN);
            break;
        case SDLK_LEFT:
            m_InputState |= static_cast<InputActionMask>(InputAction::ROTATE_LEFT);
            break;
        case SDLK_RIGHT:
            m_InputState |= static_cast<InputActionMask>(InputAction::ROTATE_RIGHT);
            break;
        case SDLK_UP:
            m_InputState |= static_cast<InputActionMask>(InputAction::ROTATE_UP);
            break;
        case SDLK_DOWN:
            m_InputState |= static_cast<InputActionMask>(InputAction::ROTATE_DOWN);
            break;
        case SDLK_h:
            m_InputState |= static_cast<InputActionMask>(InputAction::TOGGLE_CONFIG_PANEL);
            break;
        case SDLK_1:
            m_InputState |= static_cast<InputActionMask>(InputAction::TOGGLE_WIREFRAME);
            break;
        }
    }

	void InputSystem::HandleMouseMotion(const SDL_MouseMotionEvent& event)
	{
		if (!m_isDragging)
		{
			return;
		}

		// Relative motions
		int dx = event.xrel;
		int dy = event.yrel;
	}

    void InputSystem::Destroy()
    {
    }

}