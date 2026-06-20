#include "FileDialog.h"

#include <KrisLogger/Logger.h>

#include <shobjidl.h>
#include <string>
#include <vector>
#include <windows.h>

namespace RetroRenderer::NativeFileDialog {
namespace {

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (length <= 0) {
        return {};
    }
    std::wstring wideValue(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, wideValue.data(), length);
    wideValue.resize(static_cast<size_t>(length) - 1);
    return wideValue;
}

std::wstring BuildFilterPattern(const FileDialogFilter& filter) {
    std::wstring pattern;
    for (size_t i = 0; i < filter.patterns.size(); i++) {
        if (i != 0) {
            pattern += L";";
        }
        const std::string& sourcePattern = filter.patterns[i];
        if (sourcePattern.find('*') != std::string::npos || sourcePattern.find('?') != std::string::npos) {
            pattern += Utf8ToWide(sourcePattern);
        } else if (!sourcePattern.empty() && sourcePattern.front() == '.') {
            pattern += L"*";
            pattern += Utf8ToWide(sourcePattern);
        } else {
            pattern += L"*.";
            pattern += Utf8ToWide(sourcePattern);
        }
    }
    return pattern;
}

} // namespace

std::optional<std::filesystem::path> ShowOpenFileDialog(const FileDialogRequest& request) {
    const HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shouldUninitialize = SUCCEEDED(initResult);
    if (FAILED(initResult) && initResult != RPC_E_CHANGED_MODE) {
        LOGE("CoInitializeEx failed for file dialog: 0x%08lx", static_cast<unsigned long>(initResult));
        return std::nullopt;
    }

    IFileOpenDialog* dialog = nullptr;
    const HRESULT createResult = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(createResult)) {
        if (shouldUninitialize) {
            CoUninitialize();
        }
        LOGE("Failed to create native file dialog: 0x%08lx", static_cast<unsigned long>(createResult));
        return std::nullopt;
    }

    DWORD options = 0;
    if (SUCCEEDED(dialog->GetOptions(&options))) {
        dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
    }

    if (!request.title.empty()) {
        const std::wstring title = Utf8ToWide(request.title);
        dialog->SetTitle(title.c_str());
    }

    std::vector<std::wstring> filterNames;
    std::vector<std::wstring> filterPatterns;
    std::vector<COMDLG_FILTERSPEC> filterSpecs;
    filterNames.reserve(request.filters.size());
    filterPatterns.reserve(request.filters.size());
    filterSpecs.reserve(request.filters.size());
    for (const FileDialogFilter& filter : request.filters) {
        filterNames.push_back(Utf8ToWide(filter.name));
        filterPatterns.push_back(BuildFilterPattern(filter));
        filterSpecs.push_back(COMDLG_FILTERSPEC{filterNames.back().c_str(), filterPatterns.back().c_str()});
    }
    if (!filterSpecs.empty()) {
        dialog->SetFileTypes(static_cast<UINT>(filterSpecs.size()), filterSpecs.data());
    }

    if (!request.defaultLocation.empty()) {
        std::filesystem::path defaultDirectory = request.defaultLocation;
        std::error_code ec;
        if (std::filesystem::exists(defaultDirectory, ec) && std::filesystem::is_regular_file(defaultDirectory, ec)) {
            defaultDirectory = defaultDirectory.parent_path();
        }
        if (!defaultDirectory.empty()) {
            IShellItem* folder = nullptr;
            const std::wstring folderPath = defaultDirectory.wstring();
            if (SUCCEEDED(SHCreateItemFromParsingName(folderPath.c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
                dialog->SetDefaultFolder(folder);
                folder->Release();
            }
        }
    }

    const HRESULT showResult = dialog->Show(nullptr);
    if (showResult == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog->Release();
        if (shouldUninitialize) {
            CoUninitialize();
        }
        return std::nullopt;
    }
    if (FAILED(showResult)) {
        LOGE("Native file dialog failed to show: 0x%08lx", static_cast<unsigned long>(showResult));
        dialog->Release();
        if (shouldUninitialize) {
            CoUninitialize();
        }
        return std::nullopt;
    }

    IShellItem* resultItem = nullptr;
    const HRESULT resultCode = dialog->GetResult(&resultItem);
    if (FAILED(resultCode) || resultItem == nullptr) {
        dialog->Release();
        if (shouldUninitialize) {
            CoUninitialize();
        }
        LOGE("Native file dialog did not return a result: 0x%08lx", static_cast<unsigned long>(resultCode));
        return std::nullopt;
    }

    PWSTR widePath = nullptr;
    const HRESULT pathCode = resultItem->GetDisplayName(SIGDN_FILESYSPATH, &widePath);
    if (FAILED(pathCode) || widePath == nullptr) {
        resultItem->Release();
        dialog->Release();
        if (shouldUninitialize) {
            CoUninitialize();
        }
        LOGE("Native file dialog returned an invalid path: 0x%08lx", static_cast<unsigned long>(pathCode));
        return std::nullopt;
    }

    const std::filesystem::path selectedPath = std::filesystem::path(widePath);
    CoTaskMemFree(widePath);
    resultItem->Release();
    dialog->Release();
    if (shouldUninitialize) {
        CoUninitialize();
    }
    return selectedPath;
}

} // namespace RetroRenderer::NativeFileDialog
