#pragma once

#include "../Scene/Camera.h"
#include "Config.h"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace RetroRenderer {

inline constexpr const char* kExampleSceneBaselineFileName = "example-baseline.cfg";

struct ExampleSceneBaseline {
    enum class MaterialType {
        PHONG_TEXTURE = 0,
        PHONG_VERTEX_COLOR = 1,
    };

    std::optional<MaterialType> materialType;
    std::optional<std::filesystem::path> materialAsset;
    std::optional<bool> showSkybox;
    std::optional<bool> backfaceCulling;
    std::optional<bool> perspectiveCorrect;
    std::optional<glm::vec3> lightPosition;
    std::optional<CameraType> cameraType;
    std::optional<Config::GLTextureSampling> glTextureSampling;

    [[nodiscard]] bool HasOverrides() const;
    void MergeFrom(const ExampleSceneBaseline& other);
};

bool ParseExampleSceneBaselineText(std::string_view text,
                                   ExampleSceneBaseline& outBaseline,
                                   std::vector<std::string>* warnings = nullptr);

bool LoadExampleSceneBaselineForScene(const std::filesystem::path& scenePath,
                                      ExampleSceneBaseline& outBaseline,
                                      std::vector<std::string>* warnings = nullptr,
                                      std::vector<std::filesystem::path>* matchedFiles = nullptr);

} // namespace RetroRenderer
