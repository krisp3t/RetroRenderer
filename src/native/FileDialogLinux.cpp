#include "FileDialog.h"

#include <KrisLogger/Logger.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <sys/wait.h>

namespace RetroRenderer::NativeFileDialog {
namespace {

std::string QuoteForShell(const std::string& value) {
    std::string quoted = "'";
    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\"'\"'";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

bool CommandExists(const char* commandName) {
    const std::string command = "command -v " + std::string(commandName) + " >/dev/null 2>&1";
    return std::system(command.c_str()) == 0;
}

std::string JoinPatterns(const FileDialogFilter& filter, const std::string& separator) {
    std::ostringstream stream;
    for (size_t i = 0; i < filter.patterns.size(); i++) {
        if (i != 0) {
            stream << separator;
        }
        stream << filter.patterns[i];
    }
    return stream.str();
}

std::optional<std::string> RunCommand(const std::string& command, int& outExitCode) {
    std::array<char, 512> buffer{};
    std::string output;
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        outExitCode = -1;
        return std::nullopt;
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    const int status = pclose(pipe);
    if (WIFEXITED(status)) {
        outExitCode = WEXITSTATUS(status);
    } else {
        outExitCode = -1;
    }

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }
    if (output.empty()) {
        return std::nullopt;
    }
    return output;
}

std::optional<std::filesystem::path> RunZenity(const FileDialogRequest& request) {
    std::ostringstream command;
    command << "zenity --file-selection";
    if (!request.title.empty()) {
        command << " --title=" << QuoteForShell(request.title);
    }
    if (!request.defaultLocation.empty()) {
        command << " --filename=" << QuoteForShell(request.defaultLocation.string());
    }
    for (const FileDialogFilter& filter : request.filters) {
        if (filter.patterns.empty()) {
            continue;
        }
        command << " --file-filter="
                << QuoteForShell(filter.name + " | " + JoinPatterns(filter, " "));
    }
    command << " 2>/dev/null";

    int exitCode = -1;
    const std::optional<std::string> output = RunCommand(command.str(), exitCode);
    if (exitCode == 0 && output.has_value()) {
        return std::filesystem::path(*output);
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> RunKDialog(const FileDialogRequest& request) {
    std::ostringstream command;
    command << "kdialog --getopenfilename ";
    const std::string startLocation = request.defaultLocation.empty() ? std::string(".") : request.defaultLocation.string();
    command << QuoteForShell(startLocation);
    if (!request.filters.empty() && !request.filters.front().patterns.empty()) {
        command << " " << QuoteForShell(JoinPatterns(request.filters.front(), " ") + "|" + request.filters.front().name);
    }
    if (!request.title.empty()) {
        command << " --title " << QuoteForShell(request.title);
    }
    command << " 2>/dev/null";

    int exitCode = -1;
    const std::optional<std::string> output = RunCommand(command.str(), exitCode);
    if (exitCode == 0 && output.has_value()) {
        return std::filesystem::path(*output);
    }
    return std::nullopt;
}

} // namespace

std::optional<std::filesystem::path> ShowOpenFileDialog(const FileDialogRequest& request) {
    if (CommandExists("zenity")) {
        if (const auto selection = RunZenity(request)) {
            return selection;
        }
        return std::nullopt;
    }
    if (CommandExists("kdialog")) {
        return RunKDialog(request);
    }
    LOGE("No native Linux file dialog backend found. Install zenity or kdialog.");
    return std::nullopt;
}

} // namespace RetroRenderer::NativeFileDialog
