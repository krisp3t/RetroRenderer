#include "FileDialog.h"

#import <Cocoa/Cocoa.h>

namespace RetroRenderer::NativeFileDialog {
namespace {

NSString* ToNSString(const std::string& value) {
    return [NSString stringWithUTF8String:value.c_str()];
}

NSString* NormalizePattern(const std::string& pattern) {
    if (pattern.empty()) {
        return nil;
    }
    if (pattern[0] == '*' && pattern.size() > 1 && pattern[1] == '.') {
        return ToNSString(pattern.substr(2));
    }
    if (pattern[0] == '.') {
        return ToNSString(pattern.substr(1));
    }
    return ToNSString(pattern);
}

} // namespace

std::optional<std::filesystem::path> ShowOpenFileDialog(const FileDialogRequest& request) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setResolvesAliases:YES];

        if (!request.title.empty()) {
            [panel setTitle:ToNSString(request.title)];
        }

        if (!request.defaultLocation.empty()) {
            [panel setDirectoryURL:[NSURL fileURLWithPath:ToNSString(request.defaultLocation.string())]];
        }

        NSMutableArray<NSString*>* allowedFileTypes = [NSMutableArray array];
        for (const FileDialogFilter& filter : request.filters) {
            for (const std::string& pattern : filter.patterns) {
                NSString* fileType = NormalizePattern(pattern);
                if (fileType != nil) {
                    [allowedFileTypes addObject:fileType];
                }
            }
        }
        if ([allowedFileTypes count] > 0) {
            [panel setAllowedFileTypes:allowedFileTypes];
        }

        if ([panel runModal] != NSModalResponseOK) {
            return std::nullopt;
        }

        NSURL* selection = [panel URL];
        if (selection == nil) {
            return std::nullopt;
        }
        return std::filesystem::path(std::string([[selection path] UTF8String]));
    }
}

} // namespace RetroRenderer::NativeFileDialog
