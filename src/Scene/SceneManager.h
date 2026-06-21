#pragma once

#include "../Base/InputActions.h"
#include "../Renderer/RenderServices.h"
#include "AnimationTimeline.h"
#include "Camera.h"
#include "Scene.h"
#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace RetroRenderer {

class SceneManager {
  public:
    SceneManager() = default;
    ~SceneManager() = default;

    void BindDependencies(std::shared_ptr<Config> config, IRenderInvalidationSink& renderInvalidationSink);
    void ResetScene();
    bool LoadScene(const uint8_t* data, size_t size, bool append = false);
    bool LoadScene(const std::string& path, bool append = false);
    bool ProcessInput(InputActionMask actions, unsigned int deltaTime);
    void Update(unsigned int deltaTime, const glm::ivec2& renderResolution);
    void NewFrame();
    void NotifySceneMutated();

    [[nodiscard]] std::shared_ptr<Scene> GetScene() const;
    [[nodiscard]] Camera* GetCamera() const;

    [[nodiscard]] const SceneAnimationClip& GetAnimationClip() const;
    [[nodiscard]] const AnimationPlaybackState& GetAnimationPlaybackState() const;
    [[nodiscard]] int GetCurrentAnimationFrame() const;
    [[nodiscard]] double GetAnimationPlayheadFrame() const;
    [[nodiscard]] TransformPose GetEditableAnimationPoseForModel(int modelIndex) const;
    [[nodiscard]] const AnimationTrack* GetAnimationTrackForModel(int modelIndex) const;
    [[nodiscard]] bool HasAnimationKeyAtCurrentFrame(int modelIndex) const;
    [[nodiscard]] size_t GetUnresolvedAnimationTrackCount() const;
    [[nodiscard]] bool CanPersistAnimation() const;
    [[nodiscard]] std::optional<std::filesystem::path> GetCurrentScenePath() const;
    [[nodiscard]] std::filesystem::path GetAnimationSidecarPath() const;
    [[nodiscard]] const std::string& GetAnimationStatusMessage() const;

    void SetAnimationPlaying(bool playing);
    void StopAnimation();
    void StepAnimation(int deltaFrames);
    void SetAnimationPlayheadFrame(int frame);
    void SetAnimationLoop(bool loop);
    void SetAnimationFps(int fps);
    void SetAnimationFrameRange(int startFrame, int endFrame);
    void SetAnimationPreviewPoseForCurrentFrame(int modelIndex, const TransformPose& pose);
    void ClearAnimationPreviewPose();

    bool AddAnimationKeyForCurrentFrame(int modelIndex, const TransformPose& pose);
    bool UpdateAnimationKeyForCurrentFrame(int modelIndex, const TransformPose& pose);
    bool DeleteAnimationKeyForCurrentFrame(int modelIndex);

    bool SaveAnimationSidecar();
    bool ReloadAnimationSidecar();

  private:
    struct RestPoseSnapshot {
        glm::mat4 localMatrix = glm::mat4(1.0f);
        TransformPose pose{};
    };

    struct PreviewPoseState {
        int modelIndex = -1;
        int frame = 0;
        TransformPose pose{};
    };

    void ResetAnimationState();
    void CaptureRestPoses();
    void ResolveAnimationTrackBindings();
    void ApplyAnimationToScene();
    void SetScenePath(const std::optional<std::filesystem::path>& scenePath);
    void SetAnimationStatus(std::string statusMessage);
    void MarkAnimationDocumentDirty();
    void SetAnimationPlayheadFrameInternal(double frame, bool clearPreview, bool notifyScene);
    [[nodiscard]] double ClampOrWrapFrame(double frame) const;
    [[nodiscard]] std::string BuildNodePathForModel(int modelIndex) const;
    [[nodiscard]] int ResolveModelIndexFromNodePath(const std::string& nodePath) const;
    [[nodiscard]] AnimationTrack* FindAnimationTrackForModelMutable(int modelIndex);
    [[nodiscard]] AnimationTrack* FindOrCreateAnimationTrackForModel(int modelIndex);
    [[nodiscard]] TransformPose SampleAnimationPoseForModel(int modelIndex, double frame) const;
    [[nodiscard]] bool IsModelIndexValid(int modelIndex) const;

  private:
    std::shared_ptr<Config> p_Config_ = nullptr;
    IRenderInvalidationSink* p_RenderInvalidationSink_ = nullptr;
    std::shared_ptr<Scene> p_Scene = nullptr;
    std::unique_ptr<Camera> p_Camera = nullptr;
    float m_MoveFactor = 0.02f;
    float m_RotateFactor = 0.10f;

    SceneAnimationClip m_AnimationClip = MakeDefaultAnimationClip();
    AnimationPlaybackState m_AnimationPlaybackState{};
    std::vector<RestPoseSnapshot> m_RestPoseSnapshots;
    std::optional<PreviewPoseState> m_PreviewPoseState;
    std::optional<std::filesystem::path> m_CurrentScenePath;
    std::string m_AnimationStatusMessage;
};

} // namespace RetroRenderer
