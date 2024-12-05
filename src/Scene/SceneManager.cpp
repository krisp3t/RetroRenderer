#include <array>
#include "../Base/Logger.h"
#include "SceneManager.h"
#include "../Base/InputActions.h"

namespace RetroRenderer
{
    SceneManager::~SceneManager()
    {
    }

    void SceneManager::ResetScene()
    {
		LOGI("Unloading scene");
        p_Scene = nullptr;
    }

    bool SceneManager::LoadScene(const std::string& path)
    {
        p_Scene = std::make_shared<Scene>(path);
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
            // TODO: move depending on direction vector
            p_Camera->position.z -= m_MoveFactor * deltaTime;
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_BACKWARD))
        {
            LOGD("Move backward");
            p_Camera->position.z += m_MoveFactor * deltaTime;
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
        if (!p_Scene)
        {
            return;
        }
        p_Scene->FrustumCull(*p_Camera);
    }

    std::shared_ptr<Camera> SceneManager::GetCamera() const
    {
        return p_Camera;
    }

    std::shared_ptr<Scene> SceneManager::GetScene() const
    {
        return p_Scene;
    }
}