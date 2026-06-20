#include <catch2/catch_test_macros.hpp>

#include "Base/ExampleSceneCatalog.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace RetroRenderer {
namespace {

class ScopedTempDirectory {
  public:
    ScopedTempDirectory() {
        const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
        m_path_ = std::filesystem::temp_directory_path() / ("retrorenderer-example-catalog-" + std::to_string(uniqueSuffix));
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

TEST_CASE("Example scene catalog discovers scenes recursively in sorted order", "[examples][catalog]") {
    ScopedTempDirectory tempDirectory;
    WriteFile(tempDirectory.path() / "README.md", "# Root\n");
    WriteFile(tempDirectory.path() / "ps1/README.md", "ps1 readme");
    WriteFile(tempDirectory.path() / "ps1/zeta.obj", "dummy");
    WriteFile(tempDirectory.path() / "alpha/scene-a.obj", "dummy");
    WriteFile(tempDirectory.path() / "root.obj", "dummy");

    ExampleSceneCatalog catalog(tempDirectory.path(), {});
    REQUIRE(catalog.Refresh());
    REQUIRE(catalog.RootExists());

    const auto& scenes = catalog.GetScenes();
    REQUIRE(scenes.size() == 3);
    REQUIRE(scenes[0].relativePath.generic_string() == "alpha/scene-a.obj");
    REQUIRE(scenes[1].relativePath.generic_string() == "ps1/zeta.obj");
    REQUIRE(scenes[2].relativePath.generic_string() == "root.obj");

    const auto& directories = catalog.GetDirectories();
    REQUIRE(directories.size() == 3);
    REQUIRE(directories[0].readmeText == "# Root\n");

    const auto ps1DirectoryIt = std::find_if(directories.begin(), directories.end(), [](const ExampleSceneDirectory& directory) {
        return directory.relativePath.generic_string() == "ps1";
    });
    REQUIRE(ps1DirectoryIt != directories.end());
    REQUIRE(ps1DirectoryIt->readmeText == "ps1 readme");
    REQUIRE(ps1DirectoryIt->sceneIndices.size() == 1);
}

TEST_CASE("Example scene catalog handles a missing root directory", "[examples][catalog]") {
    ScopedTempDirectory tempDirectory;
    ExampleSceneCatalog catalog(tempDirectory.path() / "missing");

    REQUIRE_FALSE(catalog.Refresh());
    REQUIRE_FALSE(catalog.RootExists());
    REQUIRE(catalog.GetScenes().empty());
    REQUIRE(catalog.GetDirectories().size() == 1);
}

TEST_CASE("Example scene catalog canonicalizes and confines example paths", "[examples][catalog]") {
    ScopedTempDirectory tempDirectory;
    const std::filesystem::path rootPath = tempDirectory.path() / "examples";
    const std::filesystem::path insidePath = WriteFile(rootPath / "nested/inside.obj", "dummy");
    const std::filesystem::path outsidePath = WriteFile(tempDirectory.path() / "outside.obj", "dummy");

    const auto canonicalInside = ExampleSceneCatalog::CanonicalizeLoadablePath(rootPath, insidePath);
    REQUIRE(canonicalInside.has_value());
    REQUIRE(*canonicalInside == std::filesystem::weakly_canonical(insidePath));

    REQUIRE_FALSE(ExampleSceneCatalog::CanonicalizeLoadablePath(rootPath, rootPath / "../outside.obj").has_value());

    SECTION("symlink escapes are rejected when symlinks are supported") {
        const std::filesystem::path escapeLink = rootPath / "nested/escape.obj";
        std::error_code ec;
        std::filesystem::create_symlink(outsidePath, escapeLink, ec);
        if (ec) {
            WARN("Skipping symlink escape check because the platform could not create a symlink");
            SUCCEED();
        } else {
            REQUIRE_FALSE(ExampleSceneCatalog::CanonicalizeLoadablePath(rootPath, escapeLink).has_value());
        }
    }
}

} // namespace RetroRenderer
