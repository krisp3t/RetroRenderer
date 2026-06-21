#include "AnimationPanel.h"

#include "../Scene/Model.h"
#include "../Scene/Scene.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace RetroRenderer {
namespace {

void DrawTimeline(SceneManager& sceneManager,
                  const AnimationTrack* track,
                  const SceneAnimationClip& clip) {
    constexpr float kFrameWidth = 18.0f;
    constexpr float kCanvasHeight = 72.0f;
    constexpr float kLeftPad = 28.0f;
    constexpr float kTopPad = 20.0f;
    constexpr float kBottomPad = 16.0f;

    const int frameCount = std::max(clip.endFrame - clip.startFrame + 1, 1);
    ImGui::BeginChild("TimelineStrip", ImVec2(0.0f, 96.0f), true, ImGuiWindowFlags_HorizontalScrollbar);

    const float contentWidth = kLeftPad + static_cast<float>(frameCount) * kFrameWidth + 16.0f;
    ImGui::InvisibleButton("##timeline_canvas", ImVec2(contentWidth, kCanvasHeight));
    const ImVec2 canvasMin = ImGui::GetItemRectMin();
    const ImVec2 canvasMax = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(18, 21, 24, 255), 4.0f);

    const float stripTop = canvasMin.y + kTopPad;
    const float stripBottom = canvasMax.y - kBottomPad;
    const float stripHeight = stripBottom - stripTop;
    drawList->AddRectFilled(ImVec2(canvasMin.x + kLeftPad, stripTop),
                            ImVec2(canvasMin.x + kLeftPad + static_cast<float>(frameCount) * kFrameWidth, stripBottom),
                            IM_COL32(32, 36, 40, 255),
                            2.0f);

    for (int frame = clip.startFrame; frame <= clip.endFrame; frame++) {
        const float x = canvasMin.x + kLeftPad + static_cast<float>(frame - clip.startFrame) * kFrameWidth;
        const ImU32 lineColor = (frame % 5 == 0) ? IM_COL32(78, 90, 96, 255) : IM_COL32(52, 60, 66, 255);
        drawList->AddLine(ImVec2(x, stripTop), ImVec2(x, stripBottom), lineColor, 1.0f);
        if (frame < clip.endFrame) {
            drawList->AddText(ImVec2(x + 2.0f, canvasMin.y + 2.0f), IM_COL32(185, 195, 205, 255), std::to_string(frame).c_str());
        }
    }

    if (track != nullptr) {
        for (const TransformKeyframe& key : track->keys) {
            if (key.frame < clip.startFrame || key.frame > clip.endFrame) {
                continue;
            }
            const float markerCenterX =
                canvasMin.x + kLeftPad + (static_cast<float>(key.frame - clip.startFrame) + 0.5f) * kFrameWidth;
            const ImVec2 markerCenter(markerCenterX, stripTop + stripHeight * 0.5f);
            drawList->AddRectFilled(ImVec2(markerCenter.x - 5.0f, markerCenter.y - 9.0f),
                                    ImVec2(markerCenter.x + 5.0f, markerCenter.y + 9.0f),
                                    IM_COL32(0, 220, 220, 255),
                                    2.0f);
            drawList->AddRect(ImVec2(markerCenter.x - 5.0f, markerCenter.y - 9.0f),
                              ImVec2(markerCenter.x + 5.0f, markerCenter.y + 9.0f),
                              IM_COL32(0, 0, 0, 220),
                              2.0f);
        }
    }

    const float playheadX = canvasMin.x + kLeftPad +
                            (static_cast<float>(sceneManager.GetAnimationPlayheadFrame()) - static_cast<float>(clip.startFrame) + 0.5f) *
                                kFrameWidth;
    drawList->AddLine(ImVec2(playheadX, canvasMin.y + 1.0f), ImVec2(playheadX, canvasMax.y - 1.0f), IM_COL32(255, 80, 80, 255), 2.0f);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float localX = ImGui::GetIO().MousePos.x - (canvasMin.x + kLeftPad);
        const int scrubFrame = clip.startFrame + static_cast<int>(std::floor(localX / kFrameWidth));
        sceneManager.SetAnimationPlayheadFrame(std::clamp(scrubFrame, clip.startFrame, clip.endFrame));
    }

    ImGui::EndChild();
}

} // namespace

void AnimationPanel::Draw(EditorContext& editorContext) {
    ImGui::Begin("Animation");

    std::shared_ptr<Scene> scene = editorContext.GetScene();
    if (!scene) {
        ImGui::TextDisabled("Load a scene to create timeline animation.");
        ImGui::End();
        return;
    }

    SceneManager& sceneManager = editorContext.GetSceneManager();
    const SceneAnimationClip& clip = sceneManager.GetAnimationClip();
    const AnimationPlaybackState& playback = sceneManager.GetAnimationPlaybackState();

    ImGui::Text("%s%s", clip.name.c_str(), playback.dirty ? " *" : "");
    ImGui::SameLine();
    if (playback.playing) {
        if (ImGui::Button("Pause")) {
            sceneManager.SetAnimationPlaying(false);
        }
    } else {
        if (ImGui::Button("Play")) {
            sceneManager.SetAnimationPlaying(true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        sceneManager.StopAnimation();
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##anim_prev", ImGuiDir_Left)) {
        sceneManager.StepAnimation(-1);
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##anim_next", ImGuiDir_Right)) {
        sceneManager.StepAnimation(1);
    }

    bool loop = clip.loop;
    if (ImGui::Checkbox("Loop", &loop)) {
        sceneManager.SetAnimationLoop(loop);
    }

    int fps = clip.fps;
    if (ImGui::InputInt("FPS", &fps)) {
        sceneManager.SetAnimationFps(fps);
    }

    int startFrame = clip.startFrame;
    int endFrame = clip.endFrame;
    if (ImGui::InputInt("Start frame", &startFrame)) {
        sceneManager.SetAnimationFrameRange(startFrame, endFrame);
    }
    if (ImGui::InputInt("End frame", &endFrame)) {
        sceneManager.SetAnimationFrameRange(startFrame, endFrame);
    }

    int currentFrame = sceneManager.GetCurrentAnimationFrame();
    if (ImGui::InputInt("Current frame", &currentFrame)) {
        sceneManager.SetAnimationPlayheadFrame(currentFrame);
    }
    ImGui::Text("Playhead: %.2f", sceneManager.GetAnimationPlayheadFrame());

    if (sceneManager.CanPersistAnimation()) {
        if (ImGui::Button("Save")) {
            sceneManager.SaveAnimationSidecar();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload")) {
            sceneManager.ReloadAnimationSidecar();
        }
        ImGui::TextWrapped("Sidecar: %s", sceneManager.GetAnimationSidecarPath().generic_string().c_str());
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("Save");
        ImGui::SameLine();
        ImGui::Button("Reload");
        ImGui::EndDisabled();
        ImGui::TextDisabled("Sidecar persistence is unavailable for this scene source.");
    }

    if (!sceneManager.GetAnimationStatusMessage().empty()) {
        ImGui::TextWrapped("%s", sceneManager.GetAnimationStatusMessage().c_str());
    }

    const size_t unresolvedTrackCount = sceneManager.GetUnresolvedAnimationTrackCount();
    if (unresolvedTrackCount > 0) {
        ImGui::TextWrapped("Unresolved tracks: %zu", unresolvedTrackCount);
    }

    if (!editorContext.selectedModelIndex.has_value() ||
        *editorContext.selectedModelIndex < 0 ||
        static_cast<size_t>(*editorContext.selectedModelIndex) >= scene->GetModelCount()) {
        ImGui::Separator();
        ImGui::TextDisabled("Select a model in the scene graph to edit its animation track.");
        ImGui::End();
        return;
    }

    const int selectedModelIndex = *editorContext.selectedModelIndex;
    const Model& selectedModel = scene->GetModel(static_cast<size_t>(selectedModelIndex));
    const bool hasKeyAtFrame = sceneManager.HasAnimationKeyAtCurrentFrame(selectedModelIndex);
    const AnimationTrack* track = sceneManager.GetAnimationTrackForModel(selectedModelIndex);

    ImGui::Separator();
    ImGui::Text("Target: %s", selectedModel.GetName().c_str());

    if (!hasKeyAtFrame) {
        if (ImGui::Button("Add Key")) {
            sceneManager.AddAnimationKeyForCurrentFrame(selectedModelIndex,
                                                        sceneManager.GetEditableAnimationPoseForModel(selectedModelIndex));
        }
    } else {
        if (ImGui::Button("Update Key")) {
            sceneManager.UpdateAnimationKeyForCurrentFrame(selectedModelIndex,
                                                           sceneManager.GetEditableAnimationPoseForModel(selectedModelIndex));
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Key")) {
            sceneManager.DeleteAnimationKeyForCurrentFrame(selectedModelIndex);
        }
    }

    DrawTimeline(sceneManager, track, clip);

    ImGui::End();
}

} // namespace RetroRenderer
