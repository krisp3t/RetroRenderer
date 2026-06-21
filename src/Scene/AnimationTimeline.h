#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace RetroRenderer {

struct TransformPose {
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotationEulerDegrees = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct TransformKeyframe {
    int frame = 0;
    TransformPose pose{};
};

struct AnimationTrack {
    std::string nodePath;
    std::string displayName;
    std::vector<TransformKeyframe> keys;
    int resolvedModelIndex = -1;
};

struct SceneAnimationClip {
    std::string name = "Main";
    int fps = 12;
    int startFrame = 0;
    int endFrame = 59;
    bool loop = true;
    std::vector<AnimationTrack> tracks;
};

struct AnimationPlaybackState {
    bool playing = false;
    double playheadFrame = 0.0;
    bool dirty = false;
};

[[nodiscard]] SceneAnimationClip MakeDefaultAnimationClip();
[[nodiscard]] TransformPose SampleAnimationTrackPose(const AnimationTrack& track,
                                                    double frame,
                                                    const TransformPose& fallbackPose);
[[nodiscard]] int FindAnimationKeyIndex(const AnimationTrack& track, int frame);
bool LoadAnimationClipFromFile(const std::filesystem::path& path,
                               SceneAnimationClip& outClip,
                               std::string& outErrorMessage);
bool SaveAnimationClipToFile(const std::filesystem::path& path,
                             const SceneAnimationClip& clip,
                             std::string& outErrorMessage);

} // namespace RetroRenderer
