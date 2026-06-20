#include "ExampleSceneBaseline.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace RetroRenderer {
namespace {

std::string TrimCopy(std::string_view text) {
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }

    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }

    return std::string(text.substr(begin, end - begin));
}

std::string ToLowerCopy(std::string_view text) {
    std::string value(text);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void AddWarning(std::vector<std::string>* warnings, const std::string& message) {
    if (warnings != nullptr) {
        warnings->push_back(message);
    }
}

bool ParseBoolValue(std::string_view text, bool& outValue) {
    const std::string lowered = ToLowerCopy(text);
    if (lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on") {
        outValue = true;
        return true;
    }
    if (lowered == "false" || lowered == "0" || lowered == "no" || lowered == "off") {
        outValue = false;
        return true;
    }
    return false;
}

bool ParseLightPosition(std::string value, glm::vec3& outValue) {
    std::replace(value.begin(), value.end(), ',', ' ');
    std::istringstream stream(value);
    return static_cast<bool>(stream >> outValue.x >> outValue.y >> outValue.z);
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

} // namespace

bool ExampleSceneBaseline::HasOverrides() const {
    return materialType.has_value() ||
           showSkybox.has_value() ||
           backfaceCulling.has_value() ||
           perspectiveCorrect.has_value() ||
           lightPosition.has_value() ||
           cameraType.has_value() ||
           glTextureSampling.has_value();
}

void ExampleSceneBaseline::MergeFrom(const ExampleSceneBaseline& other) {
    if (other.materialType.has_value()) {
        materialType = other.materialType;
    }
    if (other.showSkybox.has_value()) {
        showSkybox = other.showSkybox;
    }
    if (other.backfaceCulling.has_value()) {
        backfaceCulling = other.backfaceCulling;
    }
    if (other.perspectiveCorrect.has_value()) {
        perspectiveCorrect = other.perspectiveCorrect;
    }
    if (other.lightPosition.has_value()) {
        lightPosition = other.lightPosition;
    }
    if (other.cameraType.has_value()) {
        cameraType = other.cameraType;
    }
    if (other.glTextureSampling.has_value()) {
        glTextureSampling = other.glTextureSampling;
    }
}

bool ParseExampleSceneBaselineText(std::string_view text,
                                   ExampleSceneBaseline& outBaseline,
                                   std::vector<std::string>* warnings) {
    outBaseline = {};
    bool success = true;

    std::istringstream stream{std::string(text)};
    std::string rawLine;
    size_t lineNumber = 0;
    while (std::getline(stream, rawLine)) {
        ++lineNumber;
        const size_t commentPos = rawLine.find('#');
        if (commentPos != std::string::npos) {
            rawLine.erase(commentPos);
        }

        const std::string trimmedLine = TrimCopy(rawLine);
        if (trimmedLine.empty()) {
            continue;
        }

        const size_t separatorPos = trimmedLine.find('=');
        if (separatorPos == std::string::npos) {
            AddWarning(warnings, "line " + std::to_string(lineNumber) + ": expected `key = value`");
            success = false;
            continue;
        }

        const std::string key = ToLowerCopy(TrimCopy(trimmedLine.substr(0, separatorPos)));
        const std::string value = TrimCopy(trimmedLine.substr(separatorPos + 1));
        if (key.empty()) {
            AddWarning(warnings, "line " + std::to_string(lineNumber) + ": missing key before `=`");
            success = false;
            continue;
        }
        if (value.empty()) {
            AddWarning(warnings, "line " + std::to_string(lineNumber) + ": missing value for `" + key + "`");
            success = false;
            continue;
        }

        if (key == "material_type") {
            const std::string loweredValue = ToLowerCopy(value);
            if (loweredValue == "phong_texture") {
                outBaseline.materialType = ExampleSceneBaseline::MaterialType::PHONG_TEXTURE;
            } else if (loweredValue == "phong_vertex_color") {
                outBaseline.materialType = ExampleSceneBaseline::MaterialType::PHONG_VERTEX_COLOR;
            } else {
                AddWarning(warnings, "line " + std::to_string(lineNumber) + ": invalid material_type `" + value + "`");
                success = false;
            }
            continue;
        }

        if (key == "show_skybox") {
            bool parsed = false;
            if (ParseBoolValue(value, parsed)) {
                outBaseline.showSkybox = parsed;
            } else {
                AddWarning(warnings, "line " + std::to_string(lineNumber) + ": invalid show_skybox `" + value + "`");
                success = false;
            }
            continue;
        }

        if (key == "backface_culling") {
            bool parsed = false;
            if (ParseBoolValue(value, parsed)) {
                outBaseline.backfaceCulling = parsed;
            } else {
                AddWarning(warnings, "line " + std::to_string(lineNumber) + ": invalid backface_culling `" + value + "`");
                success = false;
            }
            continue;
        }

        if (key == "perspective_correct") {
            bool parsed = false;
            if (ParseBoolValue(value, parsed)) {
                outBaseline.perspectiveCorrect = parsed;
            } else {
                AddWarning(warnings, "line " + std::to_string(lineNumber) + ": invalid perspective_correct `" + value + "`");
                success = false;
            }
            continue;
        }

        if (key == "light_position") {
            glm::vec3 parsed = glm::vec3(0.0f);
            if (ParseLightPosition(value, parsed)) {
                outBaseline.lightPosition = parsed;
            } else {
                AddWarning(warnings, "line " + std::to_string(lineNumber) + ": invalid light_position `" + value + "`");
                success = false;
            }
            continue;
        }

        if (key == "camera_type") {
            const std::string loweredValue = ToLowerCopy(value);
            if (loweredValue == "perspective") {
                outBaseline.cameraType = CameraType::PERSPECTIVE;
            } else if (loweredValue == "orthographic") {
                outBaseline.cameraType = CameraType::ORTHOGRAPHIC;
            } else {
                AddWarning(warnings, "line " + std::to_string(lineNumber) + ": invalid camera_type `" + value + "`");
                success = false;
            }
            continue;
        }

        if (key == "gl_texture_sampling") {
            const std::string loweredValue = ToLowerCopy(value);
            if (loweredValue == "filtered_mips") {
                outBaseline.glTextureSampling = Config::GLTextureSampling::FILTERED_MIPS;
            } else if (loweredValue == "retro_nearest") {
                outBaseline.glTextureSampling = Config::GLTextureSampling::RETRO_NEAREST;
            } else {
                AddWarning(warnings, "line " + std::to_string(lineNumber) + ": invalid gl_texture_sampling `" + value + "`");
                success = false;
            }
            continue;
        }

        AddWarning(warnings, "line " + std::to_string(lineNumber) + ": unknown key `" + key + "`");
        success = false;
    }

    return success;
}

bool LoadExampleSceneBaselineForScene(const std::filesystem::path& scenePath,
                                      ExampleSceneBaseline& outBaseline,
                                      std::vector<std::string>* warnings,
                                      std::vector<std::filesystem::path>* matchedFiles) {
    outBaseline = {};
    if (matchedFiles != nullptr) {
        matchedFiles->clear();
    }

    std::error_code ec;
    const std::filesystem::path canonicalScenePath = std::filesystem::weakly_canonical(scenePath, ec);
    const std::filesystem::path resolvedScenePath = ec ? scenePath : canonicalScenePath;

    std::error_code existsEc;
    if (!std::filesystem::exists(resolvedScenePath, existsEc) || !std::filesystem::is_regular_file(resolvedScenePath, existsEc)) {
        return false;
    }

    std::vector<std::filesystem::path> directories;
    for (std::filesystem::path current = resolvedScenePath.parent_path(); !current.empty();) {
        directories.push_back(current);
        const std::filesystem::path parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }
    std::reverse(directories.begin(), directories.end());

    bool foundAnyBaseline = false;
    for (const std::filesystem::path& directory : directories) {
        const std::filesystem::path baselinePath = directory / kExampleSceneBaselineFileName;
        std::error_code baselineEc;
        if (!std::filesystem::exists(baselinePath, baselineEc) || !std::filesystem::is_regular_file(baselinePath, baselineEc)) {
            continue;
        }

        foundAnyBaseline = true;
        if (matchedFiles != nullptr) {
            matchedFiles->push_back(baselinePath);
        }

        ExampleSceneBaseline partialBaseline{};
        std::vector<std::string> parseWarnings;
        ParseExampleSceneBaselineText(ReadTextFile(baselinePath), partialBaseline, &parseWarnings);
        for (const std::string& warning : parseWarnings) {
            AddWarning(warnings, baselinePath.generic_string() + ": " + warning);
        }
        outBaseline.MergeFrom(partialBaseline);
    }

    return foundAnyBaseline;
}

} // namespace RetroRenderer
