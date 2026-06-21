#include "AnimationTimeline.h"

#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace RetroRenderer {
namespace {
using json = nlohmann::json;

glm::quat EulerDegreesToQuaternion(const glm::vec3& eulerDegrees) {
    return glm::quat(glm::radians(eulerDegrees));
}

glm::vec3 QuaternionToEulerDegrees(const glm::quat& quaternion) {
    return glm::degrees(glm::eulerAngles(quaternion));
}

bool JsonArrayToVec3(const json& value, glm::vec3& outVec, const char* fieldName, std::string& outErrorMessage) {
    if (!value.is_array() || value.size() != 3) {
        outErrorMessage = std::string("Animation field `") + fieldName + "` must be an array of three numbers.";
        return false;
    }

    glm::vec3 result(0.0f);
    for (size_t i = 0; i < 3; i++) {
        if (!value[i].is_number()) {
            outErrorMessage = std::string("Animation field `") + fieldName + "` must contain only numbers.";
            return false;
        }
        result[static_cast<glm::length_t>(i)] = value[i].get<float>();
    }
    outVec = result;
    return true;
}

bool PoseFromJson(const json& value, TransformPose& outPose, std::string& outErrorMessage) {
    if (!value.is_object()) {
        outErrorMessage = "Animation pose entries must be objects.";
        return false;
    }

    if (!JsonArrayToVec3(value.value("translation", json::array({0.0f, 0.0f, 0.0f})),
                         outPose.translation,
                         "translation",
                         outErrorMessage)) {
        return false;
    }
    if (!JsonArrayToVec3(value.value("rotationEulerDegrees", json::array({0.0f, 0.0f, 0.0f})),
                         outPose.rotationEulerDegrees,
                         "rotationEulerDegrees",
                         outErrorMessage)) {
        return false;
    }
    if (!JsonArrayToVec3(value.value("scale", json::array({1.0f, 1.0f, 1.0f})),
                         outPose.scale,
                         "scale",
                         outErrorMessage)) {
        return false;
    }
    return true;
}

json ClipToJson(const SceneAnimationClip& clip) {
    json tracks = json::array();
    for (const AnimationTrack& track : clip.tracks) {
        json keys = json::array();
        for (const TransformKeyframe& key : track.keys) {
            keys.push_back(json{
                {"frame", key.frame},
                {"translation", {key.pose.translation.x, key.pose.translation.y, key.pose.translation.z}},
                {"rotationEulerDegrees",
                 {key.pose.rotationEulerDegrees.x, key.pose.rotationEulerDegrees.y, key.pose.rotationEulerDegrees.z}},
                {"scale", {key.pose.scale.x, key.pose.scale.y, key.pose.scale.z}},
            });
        }
        tracks.push_back(json{
            {"nodePath", track.nodePath},
            {"displayName", track.displayName},
            {"keys", std::move(keys)},
        });
    }

    return json{
        {"version", 1},
        {"clip",
         {
             {"name", clip.name},
             {"fps", clip.fps},
             {"startFrame", clip.startFrame},
             {"endFrame", clip.endFrame},
             {"loop", clip.loop},
         }},
        {"tracks", std::move(tracks)},
    };
}

bool ClipFromJson(const json& root, SceneAnimationClip& outClip, std::string& outErrorMessage) {
    if (!root.is_object()) {
        outErrorMessage = "Animation sidecar root must be a JSON object.";
        return false;
    }

    const int version = root.value("version", 0);
    if (version != 1) {
        outErrorMessage = "Unsupported animation sidecar version.";
        return false;
    }

    const json clipValue = root.value("clip", json::object());
    if (!clipValue.is_object()) {
        outErrorMessage = "Animation sidecar `clip` entry must be an object.";
        return false;
    }

    SceneAnimationClip clip = MakeDefaultAnimationClip();
    clip.name = clipValue.value("name", clip.name);
    clip.fps = std::max(1, clipValue.value("fps", clip.fps));
    clip.startFrame = clipValue.value("startFrame", clip.startFrame);
    clip.endFrame = clipValue.value("endFrame", clip.endFrame);
    clip.loop = clipValue.value("loop", clip.loop);
    if (clip.endFrame < clip.startFrame) {
        std::swap(clip.startFrame, clip.endFrame);
    }

    const json tracksValue = root.value("tracks", json::array());
    if (!tracksValue.is_array()) {
        outErrorMessage = "Animation sidecar `tracks` entry must be an array.";
        return false;
    }

    clip.tracks.clear();
    clip.tracks.reserve(tracksValue.size());
    for (const json& trackValue : tracksValue) {
        if (!trackValue.is_object()) {
            outErrorMessage = "Animation tracks must be objects.";
            return false;
        }

        AnimationTrack track{};
        track.nodePath = trackValue.value("nodePath", std::string{});
        track.displayName = trackValue.value("displayName", std::string{});
        if (track.nodePath.empty()) {
            outErrorMessage = "Animation tracks must contain a non-empty `nodePath`.";
            return false;
        }

        const json keysValue = trackValue.value("keys", json::array());
        if (!keysValue.is_array()) {
            outErrorMessage = "Animation track `keys` entry must be an array.";
            return false;
        }

        track.keys.reserve(keysValue.size());
        for (const json& keyValue : keysValue) {
            if (!keyValue.is_object()) {
                outErrorMessage = "Animation keys must be objects.";
                return false;
            }
            if (!keyValue.contains("frame") || !keyValue["frame"].is_number_integer()) {
                outErrorMessage = "Animation keys must contain an integer `frame`.";
                return false;
            }

            TransformKeyframe key{};
            key.frame = keyValue["frame"].get<int>();
            if (!PoseFromJson(keyValue, key.pose, outErrorMessage)) {
                return false;
            }
            track.keys.push_back(key);
        }

        std::sort(track.keys.begin(), track.keys.end(), [](const TransformKeyframe& lhs, const TransformKeyframe& rhs) {
            return lhs.frame < rhs.frame;
        });
        track.keys.erase(std::unique(track.keys.begin(),
                                     track.keys.end(),
                                     [](const TransformKeyframe& lhs, const TransformKeyframe& rhs) {
                                         return lhs.frame == rhs.frame;
                                     }),
                         track.keys.end());
        clip.tracks.push_back(std::move(track));
    }

    outClip = std::move(clip);
    return true;
}
} // namespace

SceneAnimationClip MakeDefaultAnimationClip() {
    return SceneAnimationClip{};
}

TransformPose SampleAnimationTrackPose(const AnimationTrack& track, double frame, const TransformPose& fallbackPose) {
    if (track.keys.empty()) {
        return fallbackPose;
    }
    if (track.keys.size() == 1) {
        return track.keys.front().pose;
    }

    const TransformKeyframe* previousKey = &track.keys.front();
    const TransformKeyframe* nextKey = &track.keys.back();

    if (frame <= static_cast<double>(track.keys.front().frame)) {
        return track.keys.front().pose;
    }
    if (frame >= static_cast<double>(track.keys.back().frame)) {
        return track.keys.back().pose;
    }

    for (size_t i = 1; i < track.keys.size(); i++) {
        if (frame <= static_cast<double>(track.keys[i].frame)) {
            previousKey = &track.keys[i - 1];
            nextKey = &track.keys[i];
            break;
        }
    }

    const double frameSpan = static_cast<double>(nextKey->frame - previousKey->frame);
    if (frameSpan <= 1e-6) {
        return nextKey->pose;
    }

    const float t = static_cast<float>(std::clamp((frame - static_cast<double>(previousKey->frame)) / frameSpan, 0.0, 1.0));
    const glm::quat previousRotation = EulerDegreesToQuaternion(previousKey->pose.rotationEulerDegrees);
    const glm::quat nextRotation = EulerDegreesToQuaternion(nextKey->pose.rotationEulerDegrees);
    const glm::quat blendedRotation = glm::normalize(glm::slerp(previousRotation, nextRotation, t));

    TransformPose sampledPose{};
    sampledPose.translation = glm::mix(previousKey->pose.translation, nextKey->pose.translation, t);
    sampledPose.rotationEulerDegrees = QuaternionToEulerDegrees(blendedRotation);
    sampledPose.scale = glm::mix(previousKey->pose.scale, nextKey->pose.scale, t);
    return sampledPose;
}

int FindAnimationKeyIndex(const AnimationTrack& track, int frame) {
    for (size_t i = 0; i < track.keys.size(); i++) {
        if (track.keys[i].frame == frame) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool LoadAnimationClipFromFile(const std::filesystem::path& path,
                               SceneAnimationClip& outClip,
                               std::string& outErrorMessage) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        outErrorMessage = "Could not open animation sidecar for reading.";
        return false;
    }

    json root;
    try {
        file >> root;
    } catch (const json::exception& exception) {
        outErrorMessage = std::string("Failed to parse animation sidecar JSON: ") + exception.what();
        return false;
    }

    return ClipFromJson(root, outClip, outErrorMessage);
}

bool SaveAnimationClipToFile(const std::filesystem::path& path,
                             const SceneAnimationClip& clip,
                             std::string& outErrorMessage) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        outErrorMessage = "Could not open animation sidecar for writing.";
        return false;
    }

    try {
        file << ClipToJson(clip).dump(2);
    } catch (const json::exception& exception) {
        outErrorMessage = std::string("Failed to serialize animation sidecar JSON: ") + exception.what();
        return false;
    }

    return true;
}

} // namespace RetroRenderer
