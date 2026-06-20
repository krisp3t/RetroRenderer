#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct SDL_Window;

namespace RetroRenderer::NativeFileDialog {

struct FileDialogFilter {
    std::string name;
    std::vector<std::string> patterns;
};

struct FileDialogRequest {
    SDL_Window* parentWindow = nullptr;
    std::string title;
    std::filesystem::path defaultLocation;
    std::vector<FileDialogFilter> filters;
};

[[nodiscard]] std::optional<std::filesystem::path> ShowOpenFileDialog(const FileDialogRequest& request);

} // namespace RetroRenderer::NativeFileDialog
