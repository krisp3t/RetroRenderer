#include <KrisLogger/Logger.h>
#include "SceneManager.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.inl>


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

    void SceneManager::RenderUI()
    {
        // Fixed nodes
        if (ImGui::TreeNode("Main camera"))
        {
            ImGui::Text("Main camera");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Directional light"))
        {
            ImGui::Text("Directional light");
            ImGui::TreePop();
        }

        if (p_Scene.get() == nullptr)
        {
            return;
        }
        if (ImGui::TreeNode("Scene"))
        {
            for (size_t i = 0; i < p_Scene->m_Models.size(); i++)
            {
                if (!p_Scene->m_Models[i].m_Parent.has_value()) // roots only
                {
                    RenderUIModelRecursive(i);
                }
            }
            ImGui::TreePop();
        }
    }

    void SceneManager::RenderUIModelRecursive(int modelIndex)
    {
        auto &model = p_Scene->m_Models[modelIndex];
        constexpr ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
                             | ImGuiTreeNodeFlags_OpenOnDoubleClick
                             | ImGuiTreeNodeFlags_SpanAvailWidth;
        /*
        if (scene.m_SelectedModel == modelIndex)
            nodeFlags |= ImGuiTreeNodeFlags_Selected;
        */
        bool opened = true;
        if (ImGui::TreeNode(model.GetName().c_str()))
        {
            if (opened)
            {
                glm::vec3 pos, rot, scl;
                model.GetTRS(pos, rot, scl);
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
                ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", rot.x, rot.y, rot.z);
                ImGui::Text("Scale:    (%.2f, %.2f, %.2f)", scl.x, scl.y, scl.z);
                for (int childIndex : p_Scene->m_Models[modelIndex].m_Children)
                {
                    RenderUIModelRecursive(childIndex);
                }
            }
            ImGui::TreePop();
        }
    }
}
