#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace RetroRenderer {

struct ExampleSceneEntry {
    std::filesystem::path absolutePath;
    std::filesystem::path relativePath;
    std::string displayName;
    size_t directoryIndex = 0;
};

struct ExampleSceneDirectory {
    std::filesystem::path absolutePath;
    std::filesystem::path relativePath;
    std::string displayName;
    std::vector<size_t> childDirectoryIndices;
    std::vector<size_t> sceneIndices;
    std::string readmeText;
};

class ExampleSceneCatalog {
  public:
    explicit ExampleSceneCatalog(
        std::filesystem::path rootPath = std::filesystem::path("assets"),
        std::vector<std::filesystem::path> scanRoots = {std::filesystem::path("tests-visual"),
                                                        std::filesystem::path("shader-examples")});

    bool Refresh(const std::vector<std::string>& supportedExtensions = {".obj"});

    [[nodiscard]] const std::filesystem::path& GetRootPath() const;
    [[nodiscard]] bool RootExists() const;
    [[nodiscard]] const std::vector<ExampleSceneEntry>& GetScenes() const;
    [[nodiscard]] const std::vector<ExampleSceneDirectory>& GetDirectories() const;

    [[nodiscard]] static bool IsPathWithinRoot(const std::filesystem::path& rootPath,
                                               const std::filesystem::path& candidatePath);
    [[nodiscard]] static std::optional<std::filesystem::path> CanonicalizeLoadablePath(
        const std::filesystem::path& rootPath,
        const std::filesystem::path& candidatePath);

  private:
    [[nodiscard]] size_t EnsureDirectory(const std::filesystem::path& relativeDirectoryPath);
    void RebuildDirectorySceneLists();

  private:
    std::filesystem::path m_rootPath;
    std::vector<std::filesystem::path> m_scanRoots;
    bool m_rootExists_ = false;
    std::vector<ExampleSceneEntry> m_scenes_;
    std::vector<ExampleSceneDirectory> m_directories_;
};

} // namespace RetroRenderer
