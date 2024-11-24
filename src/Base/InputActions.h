#pragma once

#include <array>
#include <utility>
#include <SDL2/SDL.h>

namespace RetroRenderer
{
    enum class InputAction
    {
        QUIT = 1 << 0,
        MOVE_FORWARD = 1 << 1,
        MOVE_BACKWARD = 1 << 2,
        MOVE_LEFT = 1 << 3,
        MOVE_RIGHT = 1 << 4,
        MOVE_UP = 1 << 5,
        MOVE_DOWN = 1 << 6,
        ROTATE_LEFT = 1 << 7,
        ROTATE_RIGHT = 1 << 8,
        ROTATE_UP = 1 << 9,
        ROTATE_DOWN = 1 << 10,
        TOGGLE_CONFIG_PANEL = 1 << 11,
        TOGGLE_WIREFRAME = 1 << 12,
    };
    using InputActionMask = int32_t;

}