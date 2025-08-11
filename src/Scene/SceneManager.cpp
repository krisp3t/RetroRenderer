#include <KrisLogger/Logger.h>
#include "SceneManager.h"


namespace RetroRenderer
{
    SceneManager::~SceneManager()
    {
    }

    void SceneManager::ResetScene()
    {
        LOGI("Unloading scene");
        p_Scene = nullptr;
        p_Camera = nullptr;
    }

    bool SceneManager::LoadScene(const uint8_t* data, const size_t size)
    {
        ResetScene();
        p_Scene = std::make_shared<Scene>();
        p_Camera = std::make_unique<Camera>();
        if (!p_Scene->Load(data, size))
        {
            p_Scene = nullptr;
            return false;
        }
        return true;
    }

    bool SceneManager::LoadScene(const std::string &path)
    {
        ResetScene();
        p_Scene = std::make_shared<Scene>();
        p_Camera = std::make_unique<Camera>();
        if (!p_Scene->Load(path))
        {
            p_Scene = nullptr;
            return false;
        }
        return true;
    }

    /**
	 * @brief Process input actions (translate, rotate camera)
	 * @return true if there was any actions to process, false otherwise
     */
    bool SceneManager::ProcessInput(InputActionMask actions, unsigned int deltaTime)
    {
        if (actions == 0 || !p_Camera)
        {
            return false;
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_FORWARD))
        {
            p_Camera->m_Position += p_Camera->m_Direction * (m_MoveFactor * deltaTime);
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_BACKWARD))
        {
            p_Camera->m_Position -= p_Camera->m_Direction * (m_MoveFactor * deltaTime);
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_LEFT))
        {
            p_Camera->m_Position -= glm::normalize(
                    glm::cross(p_Camera->m_Direction, p_Camera->m_Up)
            ) *
                                  (m_MoveFactor * deltaTime);
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_RIGHT))
        {
            p_Camera->m_Position += glm::normalize(
                    glm::cross(p_Camera->m_Direction, p_Camera->m_Up)
            ) *
                                  (m_MoveFactor * deltaTime);
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_UP))
        {
            p_Camera->m_Position += p_Camera->m_Up * (m_MoveFactor * deltaTime);
        }
        if (actions & static_cast<InputActionMask>(InputAction::MOVE_DOWN))
        {
            p_Camera->m_Position -= p_Camera->m_Up * (m_MoveFactor * deltaTime);
        }
        if (actions & static_cast<InputActionMask>(InputAction::ROTATE_LEFT))
        {
            p_Camera->m_EulerRotation.y += 1.0f * (m_RotateFactor * deltaTime); // Yaw
        }
        if (actions & static_cast<InputActionMask>(InputAction::ROTATE_RIGHT))
        {
            p_Camera->m_EulerRotation.y -= 1.0f * (m_RotateFactor * deltaTime); // Yaw
        }
        if (actions & static_cast<InputActionMask>(InputAction::ROTATE_UP))
        {
            p_Camera->m_EulerRotation.x += 1.0f * (m_RotateFactor * deltaTime); // Pitch
        }
        if (actions & static_cast<InputActionMask>(InputAction::ROTATE_DOWN))
        {
            p_Camera->m_EulerRotation.x -= 1.0f * (m_RotateFactor * deltaTime); // Pitch
        }
        return true;
    }

    void SceneManager::Update(unsigned int deltaTime)
    {
        if (!p_Scene || !p_Camera)
        {
            return;
        }
        // TODO: nullable type?
        p_Camera->UpdateViewMatrix();
    }

    void SceneManager::NewFrame()
    {
        if (!p_Scene || !p_Camera)
        {
            return;
        }
        p_Scene->FrustumCull(*p_Camera);
    }

    Camera* SceneManager::GetCamera() const
    {
        return p_Camera.get();
    }

    std::shared_ptr<Scene> SceneManager::GetScene() const
    {
        return p_Scene;
    }
}