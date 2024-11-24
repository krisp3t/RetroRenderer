#include <array>
#include "../Base/Logger.h"
#include "SceneManager.h"
#include "../Base/InputActions.h"

namespace RetroRenderer
{
    SceneManager::~SceneManager()
    {
    }

    bool SceneManager::LoadScene(const std::string& path)
    {
        m_Scene = std::make_unique<Scene>(path);
        return true;
    }

    void SceneManager::ProcessInput(InputActionMask actions, unsigned int deltaTime)
    {
        if (actions == 0)
        {
            return;
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_FORWARD))
        {
            LOGD("Move forward");
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_BACKWARD))
        {
            LOGD("Move backward");
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_LEFT))
        {
            LOGD("Move left");
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_RIGHT))
        {
            LOGD("Move right");
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_UP))
        {
            LOGD("Move up");
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_DOWN))
        {
            LOGD("Move down");
        }
        if (actions & static_cast<InputActionMask>(InputAction::ROTATE_LEFT))
        {
            LOGD("Rotate left");
        }
        if (actions & static_cast<InputActionMask>(InputAction::ROTATE_RIGHT))
        {
            LOGD("Rotate right");
        }
        if (actions & static_cast<InputActionMask>(InputAction::ROTATE_UP))
        {
            LOGD("Rotate up");
        }
        if (actions & static_cast<InputActionMask>(InputAction::ROTATE_DOWN))
        {
            LOGD("Rotate down");
        }
    }

    void SceneManager::Update(unsigned int deltaTime)
    {

    }
}