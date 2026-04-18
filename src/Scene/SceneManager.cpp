#include "SceneManager.h"
#include "../Engine.h"
#include <KrisLogger/Logger.h>

#include <glm/gtc/type_ptr.inl>
#include <imgui.h>

namespace RetroRenderer {
SceneManager::~SceneManager() {
}

void SceneManager::ResetScene() {
    LOGI("Unloading scene");
    p_Scene = nullptr;
    p_Camera = nullptr;
}

bool SceneManager::LoadScene(const uint8_t* data, const size_t size) {
    ResetScene();
    p_Scene = std::make_shared<Scene>();
    p_Camera = std::make_unique<Camera>();
    if (!p_Scene->Load(data, size)) {
        p_Scene = nullptr;
        return false;
    }
    return true;
}

bool SceneManager::LoadScene(const std::string& path) {
    ResetScene();
    p_Scene = std::make_shared<Scene>();
    p_Camera = std::make_unique<Camera>();
    if (!p_Scene->Load(path)) {
        p_Scene = nullptr;
        return false;
    }
    return true;
}

/**
 * @brief Process input actions (translate, rotate camera)
 * @return true if there was any actions to process, false otherwise
 */
bool SceneManager::ProcessInput(InputActionMask actions, unsigned int deltaTime) {
    if (actions == 0 || !p_Camera) {
        return false;
    }
    if (actions & static_cast<InputActionMask>(InputAction::MOVE_FORWARD)) {
        p_Camera->m_Position += p_Camera->m_Direction * (m_MoveFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::MOVE_BACKWARD)) {
        p_Camera->m_Position -= p_Camera->m_Direction * (m_MoveFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::MOVE_LEFT)) {
        p_Camera->m_Position -=
            glm::normalize(glm::cross(p_Camera->m_Direction, p_Camera->m_Up)) * (m_MoveFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::MOVE_RIGHT)) {
        p_Camera->m_Position +=
            glm::normalize(glm::cross(p_Camera->m_Direction, p_Camera->m_Up)) * (m_MoveFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::MOVE_UP)) {
        p_Camera->m_Position += p_Camera->m_Up * (m_MoveFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::MOVE_DOWN)) {
        p_Camera->m_Position -= p_Camera->m_Up * (m_MoveFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::ROTATE_LEFT)) {
        p_Camera->m_EulerRotation.y += 1.0f * (m_RotateFactor * deltaTime); // Yaw
    }
    if (actions & static_cast<InputActionMask>(InputAction::ROTATE_RIGHT)) {
        p_Camera->m_EulerRotation.y -= 1.0f * (m_RotateFactor * deltaTime); // Yaw
    }
    if (actions & static_cast<InputActionMask>(InputAction::ROTATE_UP)) {
        p_Camera->m_EulerRotation.x += 1.0f * (m_RotateFactor * deltaTime); // Pitch
    }
    if (actions & static_cast<InputActionMask>(InputAction::ROTATE_DOWN)) {
        p_Camera->m_EulerRotation.x -= 1.0f * (m_RotateFactor * deltaTime); // Pitch
    }
    return true;
}

void SceneManager::Update(unsigned int deltaTime) {
    if (!p_Scene || !p_Camera) {
        return;
    }
    // TODO: nullable type?
    p_Camera->UpdateViewMatrix();
}

void SceneManager::NewFrame() {
    if (!p_Scene || !p_Camera) {
        return;
    }
    p_Scene->FrustumCull(*p_Camera);
}

Camera* SceneManager::GetCamera() const {
    return p_Camera.get();
}

std::shared_ptr<Scene> SceneManager::GetScene() const {
    return p_Scene;
}

void SceneManager::RenderUI() {
    // Fixed nodes
    if (ImGui::TreeNode("Main camera")) {
        ImGui::Text("Main camera");
        ImGui::TreePop();
    }

    if (p_Scene.get() == nullptr) {
        return;
    }
    if (ImGui::TreeNode("Scene")) {
        if (ImGui::TreeNode("Lights")) {
            auto& lights = p_Scene->GetLights();
            for (size_t i = 0; i < lights.size(); i++) {
                RenderUILight(lights[i], static_cast<int>(i));
            }
            ImGui::TreePop();
        }
        for (size_t i = 0; i < p_Scene->m_Models.size(); i++) {
            if (!p_Scene->m_Models[i].m_Parent.has_value()) // roots only
            {
                RenderUIModelRecursive(i);
            }
        }
        ImGui::TreePop();
    }
}

void SceneManager::RenderUILight(SceneLight& light, int lightIndex) {
    const std::string label = light.name + "##light_" + std::to_string(lightIndex);
    if (ImGui::TreeNode(label.c_str())) {
        ImGui::Text("Type: %s", light.type == LightType::POINT ? "Point" : "Unknown");
        ImGui::DragFloat3("Position", glm::value_ptr(light.position), 0.1f, 0.0f, 0.0f, "%.3f");
        ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 4.0f, "%.2f");
        ImGui::TreePop();
    }
}

void SceneManager::RenderUIModelRecursive(int modelIndex) {
    auto& model = p_Scene->m_Models[modelIndex];
    constexpr ImGuiTreeNodeFlags nodeFlags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    /*
    if (scene.m_SelectedModel == modelIndex)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    */
    bool opened = true;
    if (ImGui::TreeNode(model.GetName().c_str())) {
        if (opened) {
            glm::vec3 lPos, lRot, lScl;
            glm::vec3 wPos, wRot, wScl;
            model.GetLocalTRS(lPos, lRot, lScl);
            bool transformChanged = false;
            transformChanged |= ImGui::DragFloat3("Local position", glm::value_ptr(lPos), 0.1f, 0.0f, 0.0f, "%.3f");
            transformChanged |= ImGui::DragFloat3("Local rotation", glm::value_ptr(lRot), 0.5f, 0.0f, 0.0f, "%.2f");
            transformChanged |= ImGui::DragFloat3("Local scale", glm::value_ptr(lScl), 0.01f, 0.0f, 0.0f, "%.3f");
            if (transformChanged) {
                model.SetLocalTRS(lPos, lRot, lScl);
            }
            if (transformChanged) {
                Engine::Get().GetRenderSystem().OnSceneMutated();
            }
            model.GetWorldTRS(wPos, wRot, wScl);
            model.GetLocalTRS(lPos, lRot, lScl);

            ImGui::Text("Local pos:      (%.2f, %.2f, %.2f)", lPos.x, lPos.y, lPos.z);
            ImGui::SameLine();
            ImGui::Text("World pos:      (%.2f, %.2f, %.2f)", wPos.x, wPos.y, wPos.z);

            ImGui::Text("Local rot:      (%.2f, %.2f, %.2f)", lRot.x, lRot.y, lRot.z);
            ImGui::SameLine();
            ImGui::Text("World rot:      (%.2f, %.2f, %.2f)", wRot.x, wRot.y, wRot.z);

            ImGui::Text("Local scale:    (%.2f, %.2f, %.2f)", lScl.x, lScl.y, lScl.z);
            ImGui::SameLine();
            ImGui::Text("World scale:    (%.2f, %.2f, %.2f)", wScl.x, wScl.y, wScl.z);

            for (int childIndex : p_Scene->m_Models[modelIndex].m_Children) {
                RenderUIModelRecursive(childIndex);
            }
        }
        ImGui::TreePop();
    }
}
} // namespace RetroRenderer
