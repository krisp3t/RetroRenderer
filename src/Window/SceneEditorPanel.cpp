#include "SceneEditorPanel.h"

#include "../Scene/Model.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneManager.h"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <string>

namespace RetroRenderer {

void SceneEditorPanel::Draw(EditorContext& editorContext) {
    ImGui::Begin("Scene Graph");
    ImGuiIO& io = ImGui::GetIO();
    ImFont* smallFont = io.Fonts->Fonts[0];
    constexpr float kScale = 0.8f;
    ImGui::PushFont(smallFont);
    ImGui::SetWindowFontScale(kScale);

    if (ImGui::TreeNode("Main camera")) {
        ImGui::Text("Main camera");
        ImGui::TreePop();
    }

    std::shared_ptr<Scene> scene = editorContext.GetScene();
    if (scene && ImGui::TreeNode("Scene")) {
        if (ImGui::TreeNode("Lights")) {
            auto& lights = scene->GetLights();
            for (size_t i = 0; i < lights.size(); ++i) {
                DrawLight(lights[i], static_cast<int>(i), editorContext);
            }
            ImGui::TreePop();
        }

        for (size_t i = 0; i < scene->GetModelCount(); ++i) {
            if (!scene->GetModel(i).GetParent().has_value()) {
                DrawModelRecursive(*scene, static_cast<int>(i), editorContext);
            }
        }
        ImGui::TreePop();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::End();
}

void SceneEditorPanel::DrawLight(SceneLight& light, int lightIndex, EditorContext& editorContext) {
    const std::string label = light.name + "##light_" + std::to_string(lightIndex);
    if (!ImGui::TreeNode(label.c_str())) {
        return;
    }

    ImGui::Text("Type: %s", light.type == LightType::POINT ? "Point" : "Unknown");
    bool lightChanged = false;
    lightChanged |= ImGui::DragFloat3("Position", glm::value_ptr(light.position), 0.1f, 0.0f, 0.0f, "%.3f");
    lightChanged |= ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
    lightChanged |= ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 4.0f, "%.2f");
    if (lightChanged) {
        editorContext.GetSceneManager().NotifySceneMutated();
    }
    ImGui::TreePop();
}

void SceneEditorPanel::DrawModelRecursive(Scene& scene, int modelIndex, EditorContext& editorContext) {
    Model& model = scene.GetModel(static_cast<size_t>(modelIndex));
    ImGuiTreeNodeFlags nodeFlags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (editorContext.selectedModelIndex.has_value() && *editorContext.selectedModelIndex == modelIndex) {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }
    const std::string label = model.GetName() + "##model_" + std::to_string(modelIndex);
    const bool opened = ImGui::TreeNodeEx(label.c_str(), nodeFlags);
    if (ImGui::IsItemClicked()) {
        if (!editorContext.selectedModelIndex.has_value() || *editorContext.selectedModelIndex != modelIndex) {
            editorContext.selectedModelIndex = modelIndex;
            editorContext.GetSceneManager().ClearAnimationPreviewPose();
        }
    }
    if (!opened) {
        return;
    }

    if (editorContext.selectedModelIndex.has_value() && *editorContext.selectedModelIndex == modelIndex) {
        TransformPose pose = editorContext.GetSceneManager().GetEditableAnimationPoseForModel(modelIndex);
        bool transformChanged = false;
        transformChanged |=
            ImGui::DragFloat3("Local position", glm::value_ptr(pose.translation), 0.1f, 0.0f, 0.0f, "%.3f");
        transformChanged |=
            ImGui::DragFloat3("Local rotation", glm::value_ptr(pose.rotationEulerDegrees), 0.5f, 0.0f, 0.0f, "%.2f");
        transformChanged |= ImGui::DragFloat3("Local scale", glm::value_ptr(pose.scale), 0.01f, 0.0f, 0.0f, "%.3f");
        if (transformChanged) {
            editorContext.GetSceneManager().SetAnimationPreviewPoseForCurrentFrame(modelIndex, pose);
        }
    }

    for (int childIndex : model.GetChildren()) {
        DrawModelRecursive(scene, childIndex, editorContext);
    }

    ImGui::TreePop();
}

} // namespace RetroRenderer
