#ifdef __EMSCRIPTEN__
#include "../native/emscripten/imgui_impl_sdl2.h"
#else
#include <imgui_impl_sdl2.h>
#endif
#include <KrisLogger/Logger.h>
#include "InputSystem.h"
#include "../Engine.h"

namespace RetroRenderer
{
    bool InputSystem::Init()
    {
        return true;
    }

    InputActionMask InputSystem::HandleInput()
    {
		auto const& p_config = Engine::Get().GetConfig();
        m_inputState_ = static_cast<InputActionMask>(0);
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
			ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
                case SDL_QUIT:
                    m_inputState_ |= static_cast<InputActionMask>(InputAction::QUIT);
                    return m_inputState_;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        m_inputState_ |= static_cast<InputActionMask>(InputAction::QUIT);
                        return m_inputState_;
                    }
                    HandleKeyDown(event.key.keysym.sym, *p_config);
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        LOGD("Window resized to %d x %d", event.window.data1, event.window.data2);
                        p_config->window.size.x = event.window.data1;
                        p_config->window.size.y = event.window.data2;
                    }
					break;
            }
        }
        return m_inputState_;
    }

    void InputSystem::HandleKeyDown(SDL_Keycode key, Config& config)
    {
        switch (key)
        {
        case SDLK_w:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_FORWARD);
            break;
        case SDLK_s:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_BACKWARD);
            break;
        case SDLK_a:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_LEFT);
            break;
        case SDLK_d:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_RIGHT);
            break;
        case SDLK_LSHIFT:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_UP);
            break;
        case SDLK_LCTRL:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::MOVE_DOWN);
            break;
        case SDLK_LEFT:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::ROTATE_LEFT);
            break;
        case SDLK_RIGHT:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::ROTATE_RIGHT);
            break;
        case SDLK_UP:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::ROTATE_UP);
            break;
        case SDLK_DOWN:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::ROTATE_DOWN);
            break;
        case SDLK_h:
			config.window.showConfigPanel = !config.window.showConfigPanel;
            // m_inputState_ |= static_cast<InputActionMask>(InputAction::TOGGLE_CONFIG_PANEL);
            break;
        case SDLK_1:
            m_inputState_ |= static_cast<InputActionMask>(InputAction::TOGGLE_WIREFRAME);
            break;
        }
    }

	void InputSystem::HandleMouseMotion(const SDL_MouseMotionEvent& event)
	{
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

    void InputSystem::Destroy()
    {
    }

}