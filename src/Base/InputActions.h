#pragma once

#include <array>
#include <utility>
#include <SDL2/SDL.h>

namespace RetroRenderer
{
    enum class InputAction
    {
        MOVE_FORWARD,
        MOVE_BACKWARD,
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_UP,
        MOVE_DOWN,
        ROTATE_LEFT,
        ROTATE_RIGHT,
        ROTATE_UP,
        ROTATE_DOWN,
        TOGGLE_CONFIG_PANEL,
        TOGGLE_WIREFRAME,
        COUNT
    };

    constexpr std::array<std::pair<InputAction, SDL_Keycode>, static_cast<size_t>(InputAction::COUNT)> InputActions = {{
        {InputAction::MOVE_FORWARD,         SDLK_w},
        {InputAction::MOVE_BACKWARD,        SDLK_s},
        {InputAction::MOVE_LEFT,            SDLK_a},
        {InputAction::MOVE_RIGHT,           SDLK_d},
        {InputAction::MOVE_UP,              SDLK_q},
        {InputAction::MOVE_DOWN,            SDLK_e},
        {InputAction::ROTATE_LEFT,          SDLK_LEFT},
        {InputAction::ROTATE_RIGHT,         SDLK_RIGHT},
        {InputAction::ROTATE_UP,            SDLK_UP},
        {InputAction::ROTATE_DOWN,          SDLK_DOWN},
        {InputAction::TOGGLE_CONFIG_PANEL,  SDLK_h},
        {InputAction::TOGGLE_WIREFRAME,     SDLK_1 }
    }};

    constexpr SDL_Keycode GetKey(InputAction action) {
        return InputActions[static_cast<size_t>(action)].second;
    }

}