#include <imgui_impl_sdl2.h>
#include "InputManager.h"
#include "../Base/InputActions.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
    bool InputManager::Init(std::shared_ptr<Config> config)
    {
        p_Config = std::move(config);
        return true;
    }

    void InputManager::HandleInput(bool &isRunning)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                HandleKeyDown(event.key.keysym.sym);
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    isRunning = false;
                }
                break;
            }
        }
    }

    void InputManager::HandleKeyDown(SDL_Keycode key)
    {
        // TODO: add deltatime
        glm::vec3& cameraPosition = p_Config->camera.position;

        switch (key)
        {
        case GetKey(InputAction::MOVE_FORWARD):
            cameraPosition.z -= 0.1f;
            break;
        case GetKey(InputAction::MOVE_BACKWARD):
            cameraPosition.z += 0.1f;
            break;
        case GetKey(InputAction::MOVE_LEFT):
            cameraPosition.x -= 0.1f;
            break;
        case GetKey(InputAction::MOVE_RIGHT):
            cameraPosition.x += 0.1f;
            break;
        case GetKey(InputAction::MOVE_UP):
            cameraPosition.y -= 0.1f;
            break;
        case GetKey(InputAction::MOVE_DOWN):
            cameraPosition.y += 0.1f;
            break;
        case GetKey(InputAction::TOGGLE_CONFIG_PANEL):
            p_Config->showConfigPanel = !p_Config->showConfigPanel;
            LOGI("Config panel enabled: %d", p_Config->showConfigPanel);
            break;
        case GetKey(InputAction::TOGGLE_WIREFRAME):
            p_Config->renderer.showWireframe = !p_Config->renderer.showWireframe;
            LOGI("Wireframe enabled: %d", p_Config->renderer.showWireframe);
            break;
        }
    }

    void InputManager::Update()
    {
    }

    void InputManager::Destroy()
    {
    }
}