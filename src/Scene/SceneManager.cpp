#include "SceneManager.h"

#include <KrisLogger/Logger.h>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace RetroRenderer {
namespace {
TransformPose PoseFromModel(const Model& model) {
    TransformPose pose{};
    model.GetLocalTRS(pose.translation, pose.rotationEulerDegrees, pose.scale);
    return pose;
}
} // namespace

void SceneManager::BindDependencies(std::shared_ptr<Config> config, IRenderInvalidationSink& renderInvalidationSink) {
    p_Config_ = std::move(config);
    p_RenderInvalidationSink_ = &renderInvalidationSink;
}

void SceneManager::ResetAnimationState() {
    m_AnimationClip = MakeDefaultAnimationClip();
    m_AnimationPlaybackState = {};
    m_AnimationPlaybackState.playheadFrame = static_cast<double>(m_AnimationClip.startFrame);
    m_PreviewPoseState.reset();
    m_AnimationStatusMessage.clear();
}

void SceneManager::SetScenePath(const std::optional<std::filesystem::path>& scenePath) {
    m_CurrentScenePath = scenePath;
}

void SceneManager::SetAnimationStatus(std::string statusMessage) {
    m_AnimationStatusMessage = std::move(statusMessage);
}

void SceneManager::ResetScene() {
    LOGI("Unloading scene");
    p_Scene = nullptr;
    p_Camera = nullptr;
    m_RestPoseSnapshots.clear();
    m_PreviewPoseState.reset();
    SetScenePath(std::nullopt);
    ResetAnimationState();
}

void SceneManager::CaptureRestPoses() {
    m_RestPoseSnapshots.clear();
    if (!p_Scene) {
        return;
    }

    m_RestPoseSnapshots.reserve(p_Scene->GetModelCount());
    for (size_t i = 0; i < p_Scene->GetModelCount(); i++) {
        const Model& model = p_Scene->GetModel(i);
        RestPoseSnapshot snapshot{};
        snapshot.localMatrix = model.GetLocalTransform();
        snapshot.pose = PoseFromModel(model);
        m_RestPoseSnapshots.push_back(snapshot);
    }
}

bool SceneManager::LoadScene(const uint8_t* data, size_t size, bool append) {
    const bool createNewScene = !append || !p_Scene || !p_Camera;
    if (createNewScene) {
        ResetScene();
        p_Scene = std::make_shared<Scene>();
        p_Scene->SetDefaultLightPosition(p_Config_->environment.lightPosition);
        p_Camera = std::make_unique<Camera>();
    }
    if (!p_Scene->Load(data, size, append && !createNewScene)) {
        if (createNewScene) {
            ResetScene();
        }
        return false;
    }

    SetScenePath(std::nullopt);
    ResetAnimationState();
    CaptureRestPoses();
    SetAnimationStatus("Animation persistence unavailable for memory-loaded scenes.");
    if (append) {
        SetAnimationStatus("Animation persistence unavailable for appended scenes.");
    }
    ApplyAnimationToScene();
    return true;
}

bool SceneManager::LoadScene(const std::string& path, bool append) {
    const bool createNewScene = !append || !p_Scene || !p_Camera;
    if (createNewScene) {
        ResetScene();
        p_Scene = std::make_shared<Scene>();
        p_Scene->SetDefaultLightPosition(p_Config_->environment.lightPosition);
        p_Camera = std::make_unique<Camera>();
    }
    if (!p_Scene->Load(path, append && !createNewScene)) {
        if (createNewScene) {
            ResetScene();
        }
        return false;
    }

    ResetAnimationState();
    CaptureRestPoses();
    if (!append) {
        SetScenePath(std::filesystem::path(path));
        ReloadAnimationSidecar();
    } else {
        SetScenePath(std::nullopt);
        SetAnimationStatus("Animation persistence unavailable for appended scenes.");
        ApplyAnimationToScene();
    }
    return true;
}

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
        p_Camera->m_EulerRotation.y += 1.0f * (m_RotateFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::ROTATE_RIGHT)) {
        p_Camera->m_EulerRotation.y -= 1.0f * (m_RotateFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::ROTATE_UP)) {
        p_Camera->m_EulerRotation.x += 1.0f * (m_RotateFactor * deltaTime);
    }
    if (actions & static_cast<InputActionMask>(InputAction::ROTATE_DOWN)) {
        p_Camera->m_EulerRotation.x -= 1.0f * (m_RotateFactor * deltaTime);
    }
    return true;
}

double SceneManager::ClampOrWrapFrame(double frame) const {
    const double startFrame = static_cast<double>(m_AnimationClip.startFrame);
    const double endFrame = static_cast<double>(m_AnimationClip.endFrame);
    if (endFrame <= startFrame) {
        return startFrame;
    }

    if (!m_AnimationClip.loop) {
        return std::clamp(frame, startFrame, endFrame);
    }

    const double span = endFrame - startFrame + 1.0;
    const double wrapped = std::fmod(frame - startFrame, span);
    return wrapped < 0.0 ? endFrame + 1.0 + wrapped : startFrame + wrapped;
}

void SceneManager::SetAnimationPlayheadFrameInternal(double frame, bool clearPreview, bool notifyScene) {
    const double normalizedFrame = ClampOrWrapFrame(frame);
    const bool frameChanged = std::abs(normalizedFrame - m_AnimationPlaybackState.playheadFrame) > 1e-4;
    const bool previewCleared = clearPreview && m_PreviewPoseState.has_value();

    if (!frameChanged && !previewCleared) {
        return;
    }

    m_AnimationPlaybackState.playheadFrame = normalizedFrame;
    if (clearPreview) {
        m_PreviewPoseState.reset();
    }
    ApplyAnimationToScene();
    if (notifyScene) {
        NotifySceneMutated();
    }
}

void SceneManager::Update(unsigned int deltaTime, const glm::ivec2& renderResolution) {
    if (!p_Scene || !p_Camera) {
        return;
    }

    if (m_AnimationPlaybackState.playing) {
        const double deltaFrames =
            static_cast<double>(deltaTime) * static_cast<double>(std::max(m_AnimationClip.fps, 1)) / 1000.0;
        if (deltaFrames > 0.0) {
            SetAnimationPlayheadFrameInternal(m_AnimationPlaybackState.playheadFrame + deltaFrames, true, true);
        }
    }

    p_Camera->UpdateViewMatrix(renderResolution);
}

void SceneManager::NewFrame() {
    if (!p_Scene || !p_Camera) {
        return;
    }
    p_Scene->FrustumCull(*p_Camera, p_Config_->cull);
}

void SceneManager::NotifySceneMutated() {
    if (p_RenderInvalidationSink_ != nullptr) {
        p_RenderInvalidationSink_->OnSceneMutated();
    }
}

std::shared_ptr<Scene> SceneManager::GetScene() const {
    return p_Scene;
}

Camera* SceneManager::GetCamera() const {
    return p_Camera.get();
}

const SceneAnimationClip& SceneManager::GetAnimationClip() const {
    return m_AnimationClip;
}

const AnimationPlaybackState& SceneManager::GetAnimationPlaybackState() const {
    return m_AnimationPlaybackState;
}

int SceneManager::GetCurrentAnimationFrame() const {
    return std::clamp(static_cast<int>(std::lround(m_AnimationPlaybackState.playheadFrame)),
                      m_AnimationClip.startFrame,
                      m_AnimationClip.endFrame);
}

double SceneManager::GetAnimationPlayheadFrame() const {
    return m_AnimationPlaybackState.playheadFrame;
}

bool SceneManager::IsModelIndexValid(int modelIndex) const {
    return p_Scene != nullptr && modelIndex >= 0 && static_cast<size_t>(modelIndex) < p_Scene->GetModelCount();
}

std::string SceneManager::BuildNodePathForModel(int modelIndex) const {
    if (!IsModelIndexValid(modelIndex)) {
        return {};
    }

    std::vector<int> segments;
    int currentIndex = modelIndex;
    while (true) {
        const Model& currentModel = p_Scene->GetModel(static_cast<size_t>(currentIndex));
        if (currentModel.GetParent().has_value()) {
            const int parentIndex = *currentModel.GetParent();
            const Model& parentModel = p_Scene->GetModel(static_cast<size_t>(parentIndex));
            const auto& children = parentModel.GetChildren();
            const auto it = std::find(children.begin(), children.end(), currentIndex);
            if (it == children.end()) {
                return {};
            }
            segments.push_back(static_cast<int>(std::distance(children.begin(), it)));
            currentIndex = parentIndex;
            continue;
        }

        int rootOrdinal = 0;
        bool foundRoot = false;
        for (size_t i = 0; i < p_Scene->GetModelCount(); i++) {
            const Model& rootCandidate = p_Scene->GetModel(i);
            if (rootCandidate.GetParent().has_value()) {
                continue;
            }
            if (static_cast<int>(i) == currentIndex) {
                segments.push_back(rootOrdinal);
                foundRoot = true;
                break;
            }
            rootOrdinal++;
        }
        if (!foundRoot) {
            return {};
        }
        break;
    }

    std::ostringstream pathBuilder;
    for (auto it = segments.rbegin(); it != segments.rend(); ++it) {
        if (it != segments.rbegin()) {
            pathBuilder << '/';
        }
        pathBuilder << *it;
    }
    return pathBuilder.str();
}

int SceneManager::ResolveModelIndexFromNodePath(const std::string& nodePath) const {
    if (!p_Scene || nodePath.empty()) {
        return -1;
    }

    std::vector<int> segments;
    std::stringstream stream(nodePath);
    std::string segmentText;
    while (std::getline(stream, segmentText, '/')) {
        if (segmentText.empty()) {
            return -1;
        }
        try {
            segments.push_back(std::stoi(segmentText));
        } catch (const std::exception&) {
            return -1;
        }
    }
    if (segments.empty()) {
        return -1;
    }

    int currentIndex = -1;
    int rootOrdinal = 0;
    for (size_t i = 0; i < p_Scene->GetModelCount(); i++) {
        const Model& rootCandidate = p_Scene->GetModel(i);
        if (rootCandidate.GetParent().has_value()) {
            continue;
        }
        if (rootOrdinal == segments.front()) {
            currentIndex = static_cast<int>(i);
            break;
        }
        rootOrdinal++;
    }
    if (currentIndex < 0) {
        return -1;
    }

    for (size_t i = 1; i < segments.size(); i++) {
        const Model& currentModel = p_Scene->GetModel(static_cast<size_t>(currentIndex));
        const auto& children = currentModel.GetChildren();
        const int childOrdinal = segments[i];
        if (childOrdinal < 0 || static_cast<size_t>(childOrdinal) >= children.size()) {
            return -1;
        }
        currentIndex = children[static_cast<size_t>(childOrdinal)];
    }

    return currentIndex;
}

void SceneManager::ResolveAnimationTrackBindings() {
    for (AnimationTrack& track : m_AnimationClip.tracks) {
        track.resolvedModelIndex = ResolveModelIndexFromNodePath(track.nodePath);
    }
}

AnimationTrack* SceneManager::FindAnimationTrackForModelMutable(int modelIndex) {
    if (!IsModelIndexValid(modelIndex)) {
        return nullptr;
    }

    for (AnimationTrack& track : m_AnimationClip.tracks) {
        if (track.resolvedModelIndex == modelIndex) {
            return &track;
        }
    }
    return nullptr;
}

const AnimationTrack* SceneManager::GetAnimationTrackForModel(int modelIndex) const {
    if (!IsModelIndexValid(modelIndex)) {
        return nullptr;
    }

    for (const AnimationTrack& track : m_AnimationClip.tracks) {
        if (track.resolvedModelIndex == modelIndex) {
            return &track;
        }
    }
    return nullptr;
}

AnimationTrack* SceneManager::FindOrCreateAnimationTrackForModel(int modelIndex) {
    if (!IsModelIndexValid(modelIndex)) {
        return nullptr;
    }

    if (AnimationTrack* existingTrack = FindAnimationTrackForModelMutable(modelIndex)) {
        existingTrack->displayName = p_Scene->GetModel(static_cast<size_t>(modelIndex)).GetName();
        return existingTrack;
    }

    AnimationTrack track{};
    track.nodePath = BuildNodePathForModel(modelIndex);
    track.displayName = p_Scene->GetModel(static_cast<size_t>(modelIndex)).GetName();
    track.resolvedModelIndex = modelIndex;
    m_AnimationClip.tracks.push_back(std::move(track));
    return &m_AnimationClip.tracks.back();
}

TransformPose SceneManager::SampleAnimationPoseForModel(int modelIndex, double frame) const {
    if (!IsModelIndexValid(modelIndex)) {
        return {};
    }

    const TransformPose fallbackPose =
        static_cast<size_t>(modelIndex) < m_RestPoseSnapshots.size() ? m_RestPoseSnapshots[static_cast<size_t>(modelIndex)].pose : TransformPose{};
    const AnimationTrack* track = GetAnimationTrackForModel(modelIndex);
    if (!track) {
        return fallbackPose;
    }
    return SampleAnimationTrackPose(*track, frame, fallbackPose);
}

void SceneManager::ApplyAnimationToScene() {
    if (!p_Scene) {
        return;
    }

    const int previewFrame = GetCurrentAnimationFrame();
    for (size_t i = 0; i < p_Scene->GetModelCount(); i++) {
        Model& model = p_Scene->GetModel(i);
        const int modelIndex = static_cast<int>(i);
        const bool hasPreviewOverride = m_PreviewPoseState.has_value() && m_PreviewPoseState->modelIndex == modelIndex &&
                                        m_PreviewPoseState->frame == previewFrame;
        const AnimationTrack* track = GetAnimationTrackForModel(modelIndex);

        if (hasPreviewOverride) {
            const TransformPose& pose = m_PreviewPoseState->pose;
            model.SetLocalTRS(pose.translation, pose.rotationEulerDegrees, pose.scale);
            continue;
        }

        if (track && !track->keys.empty()) {
            const TransformPose pose = SampleAnimationTrackPose(*track,
                                                                m_AnimationPlaybackState.playheadFrame,
                                                                m_RestPoseSnapshots[i].pose);
            model.SetLocalTRS(pose.translation, pose.rotationEulerDegrees, pose.scale);
            continue;
        }

        if (i < m_RestPoseSnapshots.size()) {
            model.SetLocalTransform(m_RestPoseSnapshots[i].localMatrix);
        }
    }
}

TransformPose SceneManager::GetEditableAnimationPoseForModel(int modelIndex) const {
    const int currentFrame = GetCurrentAnimationFrame();
    if (m_PreviewPoseState.has_value() && m_PreviewPoseState->modelIndex == modelIndex && m_PreviewPoseState->frame == currentFrame) {
        return m_PreviewPoseState->pose;
    }
    return SampleAnimationPoseForModel(modelIndex, static_cast<double>(currentFrame));
}

bool SceneManager::HasAnimationKeyAtCurrentFrame(int modelIndex) const {
    const AnimationTrack* track = GetAnimationTrackForModel(modelIndex);
    return track != nullptr && FindAnimationKeyIndex(*track, GetCurrentAnimationFrame()) >= 0;
}

size_t SceneManager::GetUnresolvedAnimationTrackCount() const {
    return static_cast<size_t>(std::count_if(
        m_AnimationClip.tracks.begin(), m_AnimationClip.tracks.end(), [](const AnimationTrack& track) {
            return track.resolvedModelIndex < 0;
        }));
}

bool SceneManager::CanPersistAnimation() const {
    return m_CurrentScenePath.has_value();
}

std::optional<std::filesystem::path> SceneManager::GetCurrentScenePath() const {
    return m_CurrentScenePath;
}

std::filesystem::path SceneManager::GetAnimationSidecarPath() const {
    if (!m_CurrentScenePath.has_value()) {
        return {};
    }
    std::filesystem::path path = *m_CurrentScenePath;
    path.replace_extension(".rranim.json");
    return path;
}

const std::string& SceneManager::GetAnimationStatusMessage() const {
    return m_AnimationStatusMessage;
}

void SceneManager::SetAnimationPlaying(bool playing) {
    if (m_AnimationPlaybackState.playing == playing) {
        return;
    }
    m_AnimationPlaybackState.playing = playing;
    if (playing) {
        const bool hadPreview = m_PreviewPoseState.has_value();
        m_PreviewPoseState.reset();
        if (hadPreview) {
            ApplyAnimationToScene();
            NotifySceneMutated();
        }
    }
}

void SceneManager::StopAnimation() {
    m_AnimationPlaybackState.playing = false;
    SetAnimationPlayheadFrameInternal(static_cast<double>(m_AnimationClip.startFrame), true, true);
}

void SceneManager::StepAnimation(int deltaFrames) {
    m_AnimationPlaybackState.playing = false;
    SetAnimationPlayheadFrameInternal(static_cast<double>(GetCurrentAnimationFrame() + deltaFrames), true, true);
}

void SceneManager::SetAnimationPlayheadFrame(int frame) {
    m_AnimationPlaybackState.playing = false;
    SetAnimationPlayheadFrameInternal(static_cast<double>(frame), true, true);
}

void SceneManager::SetAnimationLoop(bool loop) {
    if (m_AnimationClip.loop == loop) {
        return;
    }
    m_AnimationClip.loop = loop;
    MarkAnimationDocumentDirty();
    SetAnimationPlayheadFrameInternal(m_AnimationPlaybackState.playheadFrame, false, true);
}

void SceneManager::SetAnimationFps(int fps) {
    const int clampedFps = std::max(1, fps);
    if (m_AnimationClip.fps == clampedFps) {
        return;
    }
    m_AnimationClip.fps = clampedFps;
    MarkAnimationDocumentDirty();
}

void SceneManager::SetAnimationFrameRange(int startFrame, int endFrame) {
    if (endFrame < startFrame) {
        std::swap(startFrame, endFrame);
    }
    if (m_AnimationClip.startFrame == startFrame && m_AnimationClip.endFrame == endFrame) {
        return;
    }
    m_AnimationClip.startFrame = startFrame;
    m_AnimationClip.endFrame = endFrame;
    MarkAnimationDocumentDirty();
    SetAnimationPlayheadFrameInternal(m_AnimationPlaybackState.playheadFrame, true, true);
}

void SceneManager::SetAnimationPreviewPoseForCurrentFrame(int modelIndex, const TransformPose& pose) {
    if (!IsModelIndexValid(modelIndex)) {
        return;
    }

    m_AnimationPlaybackState.playing = false;
    m_PreviewPoseState = PreviewPoseState{
        .modelIndex = modelIndex,
        .frame = GetCurrentAnimationFrame(),
        .pose = pose,
    };
    ApplyAnimationToScene();
    NotifySceneMutated();
}

void SceneManager::ClearAnimationPreviewPose() {
    if (!m_PreviewPoseState.has_value()) {
        return;
    }
    m_PreviewPoseState.reset();
    ApplyAnimationToScene();
    NotifySceneMutated();
}

void SceneManager::MarkAnimationDocumentDirty() {
    m_AnimationPlaybackState.dirty = true;
}

bool SceneManager::AddAnimationKeyForCurrentFrame(int modelIndex, const TransformPose& pose) {
    AnimationTrack* track = FindOrCreateAnimationTrackForModel(modelIndex);
    if (!track) {
        return false;
    }

    const int currentFrame = GetCurrentAnimationFrame();
    if (FindAnimationKeyIndex(*track, currentFrame) >= 0) {
        return false;
    }

    track->keys.push_back(TransformKeyframe{.frame = currentFrame, .pose = pose});
    std::sort(track->keys.begin(), track->keys.end(), [](const TransformKeyframe& lhs, const TransformKeyframe& rhs) {
        return lhs.frame < rhs.frame;
    });
    m_PreviewPoseState = PreviewPoseState{.modelIndex = modelIndex, .frame = currentFrame, .pose = pose};
    MarkAnimationDocumentDirty();
    ApplyAnimationToScene();
    NotifySceneMutated();
    return true;
}

bool SceneManager::UpdateAnimationKeyForCurrentFrame(int modelIndex, const TransformPose& pose) {
    AnimationTrack* track = FindAnimationTrackForModelMutable(modelIndex);
    if (!track) {
        return false;
    }

    const int keyIndex = FindAnimationKeyIndex(*track, GetCurrentAnimationFrame());
    if (keyIndex < 0) {
        return false;
    }

    track->keys[static_cast<size_t>(keyIndex)].pose = pose;
    m_PreviewPoseState = PreviewPoseState{.modelIndex = modelIndex, .frame = GetCurrentAnimationFrame(), .pose = pose};
    MarkAnimationDocumentDirty();
    ApplyAnimationToScene();
    NotifySceneMutated();
    return true;
}

bool SceneManager::DeleteAnimationKeyForCurrentFrame(int modelIndex) {
    if (!IsModelIndexValid(modelIndex)) {
        return false;
    }

    const int currentFrame = GetCurrentAnimationFrame();
    for (auto it = m_AnimationClip.tracks.begin(); it != m_AnimationClip.tracks.end(); ++it) {
        if (it->resolvedModelIndex != modelIndex) {
            continue;
        }

        const int keyIndex = FindAnimationKeyIndex(*it, currentFrame);
        if (keyIndex < 0) {
            return false;
        }

        it->keys.erase(it->keys.begin() + keyIndex);
        if (it->keys.empty()) {
            m_AnimationClip.tracks.erase(it);
        }
        m_PreviewPoseState.reset();
        MarkAnimationDocumentDirty();
        ApplyAnimationToScene();
        NotifySceneMutated();
        return true;
    }
    return false;
}

bool SceneManager::SaveAnimationSidecar() {
    if (!CanPersistAnimation()) {
        SetAnimationStatus("Animation sidecar save is unavailable for the current scene.");
        return false;
    }

    std::string errorMessage;
    const std::filesystem::path sidecarPath = GetAnimationSidecarPath();
    if (!SaveAnimationClipToFile(sidecarPath, m_AnimationClip, errorMessage)) {
        SetAnimationStatus("Failed to save animation sidecar: " + errorMessage);
        return false;
    }

    m_AnimationPlaybackState.dirty = false;
    SetAnimationStatus("Saved animation sidecar to " + sidecarPath.filename().string());
    return true;
}

bool SceneManager::ReloadAnimationSidecar() {
    ResetAnimationState();
    if (!CanPersistAnimation()) {
        SetAnimationStatus("Animation sidecar load is unavailable for the current scene.");
        ApplyAnimationToScene();
        return false;
    }

    const std::filesystem::path sidecarPath = GetAnimationSidecarPath();
    if (!std::filesystem::exists(sidecarPath)) {
        SetAnimationStatus("No animation sidecar found.");
        ApplyAnimationToScene();
        NotifySceneMutated();
        return false;
    }

    SceneAnimationClip loadedClip = MakeDefaultAnimationClip();
    std::string errorMessage;
    if (!LoadAnimationClipFromFile(sidecarPath, loadedClip, errorMessage)) {
        SetAnimationStatus("Failed to load animation sidecar: " + errorMessage);
        ApplyAnimationToScene();
        NotifySceneMutated();
        return false;
    }

    m_AnimationClip = std::move(loadedClip);
    m_AnimationPlaybackState.playheadFrame = static_cast<double>(m_AnimationClip.startFrame);
    m_AnimationPlaybackState.dirty = false;
    ResolveAnimationTrackBindings();
    ApplyAnimationToScene();
    const size_t unresolvedCount = GetUnresolvedAnimationTrackCount();
    if (unresolvedCount > 0) {
        SetAnimationStatus("Loaded animation sidecar with " + std::to_string(unresolvedCount) + " unresolved track(s).");
    } else {
        SetAnimationStatus("Loaded animation sidecar from " + sidecarPath.filename().string());
    }
    NotifySceneMutated();
    return true;
}

} // namespace RetroRenderer
