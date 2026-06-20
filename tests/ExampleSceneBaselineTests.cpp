#include <catch2/catch_test_macros.hpp>

#include "Base/ExampleSceneBaseline.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace RetroRenderer {
namespace {

class ScopedTempDirectory {
  public:
    ScopedTempDirectory() {
        const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
        m_path_ = std::filesystem::temp_directory_path() / ("retrorenderer-example-baseline-" + std::to_string(uniqueSuffix));
        std::filesystem::create_directories(m_path_);
    }

    ~ScopedTempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(m_path_, ec);
    }

    [[nodiscard]] const std::filesystem::path& path() const {
        return m_path_;
    }

  private:
    std::filesystem::path m_path_;
};

std::filesystem::path WriteFile(const std::filesystem::path& path, const std::string& contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << contents;
    return path;
}

} // namespace

TEST_CASE("Example scene baseline parser reads supported keys", "[examples][baseline]") {
    ExampleSceneBaseline baseline{};
    std::vector<std::string> warnings;
    REQUIRE(ParseExampleSceneBaselineText(
        "# comment\n"
        "material_type = phong_vertex_color\n"
        "show_skybox = true\n"
        "backface_culling = false\n"
        "perspective_correct = true\n"
        "light_position = 1.0 2.0 -3.5\n"
        "camera_type = orthographic\n"
        "gl_texture_sampling = retro_nearest\n",
        baseline,
        &warnings));

    REQUIRE(warnings.empty());
    REQUIRE(baseline.materialType == ExampleSceneBaseline::MaterialType::PHONG_VERTEX_COLOR);
    REQUIRE(baseline.showSkybox == true);
    REQUIRE(baseline.backfaceCulling == false);
    REQUIRE(baseline.perspectiveCorrect == true);
    REQUIRE(baseline.lightPosition.has_value());
    REQUIRE(baseline.lightPosition->x == 1.0f);
    REQUIRE(baseline.lightPosition->y == 2.0f);
    REQUIRE(baseline.lightPosition->z == -3.5f);
    REQUIRE(baseline.cameraType == CameraType::ORTHOGRAPHIC);
    REQUIRE(baseline.glTextureSampling == Config::GLTextureSampling::RETRO_NEAREST);
}

TEST_CASE("Example scene baseline loader merges ancestor baselines root to leaf", "[examples][baseline]") {
    ScopedTempDirectory tempDirectory;
    const std::filesystem::path rootPath = tempDirectory.path() / "assets";
    WriteFile(rootPath / kExampleSceneBaselineFileName, "show_skybox = false\ngl_texture_sampling = filtered_mips\n");
    WriteFile(rootPath / "shader-examples/basic-tests/culling" / kExampleSceneBaselineFileName, "backface_culling = true\n");
    const std::filesystem::path sceneDirectory = rootPath / "shader-examples/basic-tests/culling/12-backface-culling";
    WriteFile(sceneDirectory / kExampleSceneBaselineFileName, "show_skybox = true\n");
    const std::filesystem::path scenePath = WriteFile(sceneDirectory / "model.obj", "# dummy\n");

    ExampleSceneBaseline baseline{};
    std::vector<std::string> warnings;
    std::vector<std::filesystem::path> matchedFiles;
    REQUIRE(LoadExampleSceneBaselineForScene(scenePath, baseline, &warnings, &matchedFiles));

    REQUIRE(warnings.empty());
    REQUIRE(matchedFiles.size() >= 3);
    REQUIRE(std::find(matchedFiles.begin(), matchedFiles.end(), rootPath / kExampleSceneBaselineFileName) != matchedFiles.end());
    REQUIRE(std::find(matchedFiles.begin(),
                      matchedFiles.end(),
                      rootPath / "shader-examples/basic-tests/culling" / kExampleSceneBaselineFileName) != matchedFiles.end());
    REQUIRE(std::find(matchedFiles.begin(),
                      matchedFiles.end(),
                      sceneDirectory / kExampleSceneBaselineFileName) != matchedFiles.end());
    REQUIRE(baseline.showSkybox == true);
    REQUIRE(baseline.backfaceCulling == true);
    REQUIRE(baseline.glTextureSampling == Config::GLTextureSampling::FILTERED_MIPS);
}

TEST_CASE("Example scene baseline parser reports invalid lines and unknown keys", "[examples][baseline]") {
    ExampleSceneBaseline baseline{};
    std::vector<std::string> warnings;
    REQUIRE_FALSE(ParseExampleSceneBaselineText(
        "show_skybox = maybe\n"
        "unknown_key = true\n"
        "light_position = nope\n"
        "not even a setting\n",
        baseline,
        &warnings));

    REQUIRE(warnings.size() == 4);
    REQUIRE_FALSE(baseline.showSkybox.has_value());
    REQUIRE_FALSE(baseline.lightPosition.has_value());
}

} // namespace RetroRenderer
