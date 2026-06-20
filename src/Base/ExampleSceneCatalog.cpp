#include "ExampleSceneCatalog.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <system_error>

namespace RetroRenderer {
namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

} // namespace

ExampleSceneCatalog::ExampleSceneCatalog(std::filesystem::path rootPath, std::vector<std::filesystem::path> scanRoots)
    : m_rootPath(std::move(rootPath)),
      m_scanRoots(std::move(scanRoots)) {
}

bool ExampleSceneCatalog::Refresh(const std::vector<std::string>& supportedExtensions) {
    m_scenes_.clear();
    m_directories_.clear();
    m_rootExists_ = false;

    ExampleSceneDirectory rootDirectory{};
    rootDirectory.absolutePath = m_rootPath;
    rootDirectory.relativePath.clear();
    rootDirectory.displayName = m_rootPath.filename().empty() ? m_rootPath.generic_string() : m_rootPath.filename().string();
    m_directories_.push_back(std::move(rootDirectory));

    std::error_code ec;
    if (!std::filesystem::exists(m_rootPath, ec) || !std::filesystem::is_directory(m_rootPath, ec)) {
        return false;
    }
    m_rootExists_ = true;
    m_directories_[0].readmeText = ReadTextFile(m_rootPath / "README.md");

    std::vector<std::string> normalizedExtensions;
    normalizedExtensions.reserve(supportedExtensions.size());
    for (const std::string& extension : supportedExtensions) {
        normalizedExtensions.push_back(ToLower(extension));
    }

    const auto scanRoot = [this, &normalizedExtensions](const std::filesystem::path& absoluteScanRoot,
                                                        const std::filesystem::path& relativeScanRoot) {
        std::error_code scanEc;
        if (!std::filesystem::exists(absoluteScanRoot, scanEc) || !std::filesystem::is_directory(absoluteScanRoot, scanEc)) {
            return;
        }

        (void)EnsureDirectory(relativeScanRoot);

        std::filesystem::recursive_directory_iterator iterator(
            absoluteScanRoot,
            std::filesystem::directory_options::skip_permission_denied,
            scanEc);
        if (scanEc) {
            return;
        }

        for (const std::filesystem::directory_entry& entry : iterator) {
            if (!entry.is_regular_file(scanEc)) {
                scanEc.clear();
                continue;
            }

            const std::string extension = ToLower(entry.path().extension().string());
            if (std::find(normalizedExtensions.begin(), normalizedExtensions.end(), extension) == normalizedExtensions.end()) {
                continue;
            }

            const std::filesystem::path relativePath = std::filesystem::relative(entry.path(), m_rootPath, scanEc);
            if (scanEc) {
                scanEc.clear();
                continue;
            }

            ExampleSceneEntry sceneEntry{};
            sceneEntry.absolutePath = entry.path();
            sceneEntry.relativePath = relativePath;
            sceneEntry.displayName = entry.path().filename().string();
            sceneEntry.directoryIndex = EnsureDirectory(relativePath.parent_path());
            m_scenes_.push_back(std::move(sceneEntry));
        }
    };

    if (m_scanRoots.empty()) {
        scanRoot(m_rootPath, {});
    } else {
        for (const std::filesystem::path& relativeScanRoot : m_scanRoots) {
            scanRoot(m_rootPath / relativeScanRoot, relativeScanRoot);
        }
    }

    std::sort(m_scenes_.begin(), m_scenes_.end(), [](const ExampleSceneEntry& lhs, const ExampleSceneEntry& rhs) {
        return lhs.relativePath.generic_string() < rhs.relativePath.generic_string();
    });

    RebuildDirectorySceneLists();
    return true;
}

const std::filesystem::path& ExampleSceneCatalog::GetRootPath() const {
    return m_rootPath;
}

bool ExampleSceneCatalog::RootExists() const {
    return m_rootExists_;
}

const std::vector<ExampleSceneEntry>& ExampleSceneCatalog::GetScenes() const {
    return m_scenes_;
}

const std::vector<ExampleSceneDirectory>& ExampleSceneCatalog::GetDirectories() const {
    return m_directories_;
}

bool ExampleSceneCatalog::IsPathWithinRoot(const std::filesystem::path& rootPath, const std::filesystem::path& candidatePath) {
    std::error_code rootEc;
    std::error_code candidateEc;
    const std::filesystem::path canonicalRoot = std::filesystem::weakly_canonical(rootPath, rootEc);
    const std::filesystem::path canonicalCandidate = std::filesystem::weakly_canonical(candidatePath, candidateEc);
    if (rootEc || candidateEc) {
        return false;
    }

    auto rootIt = canonicalRoot.begin();
    auto candidateIt = canonicalCandidate.begin();
    for (; rootIt != canonicalRoot.end() && candidateIt != canonicalCandidate.end(); ++rootIt, ++candidateIt) {
        if (*rootIt != *candidateIt) {
            return false;
        }
    }
    return rootIt == canonicalRoot.end();
}

std::optional<std::filesystem::path> ExampleSceneCatalog::CanonicalizeLoadablePath(const std::filesystem::path& rootPath,
                                                                                    const std::filesystem::path& candidatePath) {
    std::error_code existsEc;
    if (!std::filesystem::exists(candidatePath, existsEc) || !std::filesystem::is_regular_file(candidatePath, existsEc)) {
        return std::nullopt;
    }
    if (!IsPathWithinRoot(rootPath, candidatePath)) {
        return std::nullopt;
    }
    std::error_code canonicalEc;
    const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(candidatePath, canonicalEc);
    if (canonicalEc) {
        return std::nullopt;
    }
    return canonicalPath;
}

size_t ExampleSceneCatalog::EnsureDirectory(const std::filesystem::path& relativeDirectoryPath) {
    if (relativeDirectoryPath.empty() || relativeDirectoryPath == ".") {
        return 0;
    }

    const std::string relativeKey = relativeDirectoryPath.generic_string();
    for (size_t i = 0; i < m_directories_.size(); i++) {
        if (m_directories_[i].relativePath.generic_string() == relativeKey) {
            return i;
        }
    }

    const size_t parentIndex = EnsureDirectory(relativeDirectoryPath.parent_path());

    ExampleSceneDirectory directory{};
    directory.relativePath = relativeDirectoryPath;
    directory.absolutePath = m_rootPath / relativeDirectoryPath;
    directory.displayName = relativeDirectoryPath.filename().string();
    directory.readmeText = ReadTextFile(directory.absolutePath / "README.md");
    const size_t directoryIndex = m_directories_.size();
    m_directories_.push_back(std::move(directory));
    m_directories_[parentIndex].childDirectoryIndices.push_back(directoryIndex);
    return directoryIndex;
}

void ExampleSceneCatalog::RebuildDirectorySceneLists() {
    for (ExampleSceneDirectory& directory : m_directories_) {
        directory.sceneIndices.clear();
        std::sort(directory.childDirectoryIndices.begin(),
                  directory.childDirectoryIndices.end(),
                  [this](size_t lhs, size_t rhs) {
                      return m_directories_[lhs].relativePath.generic_string() < m_directories_[rhs].relativePath.generic_string();
                  });
    }

    for (size_t sceneIndex = 0; sceneIndex < m_scenes_.size(); sceneIndex++) {
        m_directories_[m_scenes_[sceneIndex].directoryIndex].sceneIndices.push_back(sceneIndex);
    }
}

} // namespace RetroRenderer
