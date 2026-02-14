#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace RetroRenderer::TestGolden {

inline std::string Trim(std::string value) {
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), [](unsigned char c) { return !std::isspace(c); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char c) { return !std::isspace(c); }).base(),
                value.end());
    return value;
}

inline bool ShouldUpdateGoldens() {
    const char* env = std::getenv("RETRO_UPDATE_GOLDENS");
    if (!env) {
        return false;
    }
    const std::string value = Trim(std::string(env));
    return value == "1" || value == "true" || value == "TRUE" || value == "on" || value == "ON";
}

inline std::map<std::string, uint64_t> LoadGoldenHashes(const std::string& filePath) {
    std::map<std::string, uint64_t> hashes;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return hashes;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = Trim(line.substr(0, separator));
        const std::string value = Trim(line.substr(separator + 1));
        if (key.empty() || value.empty()) {
            continue;
        }

        std::istringstream parser(value);
        uint64_t hashValue = 0;
        parser >> hashValue;
        if (!parser.fail()) {
            hashes[key] = hashValue;
        }
    }
    return hashes;
}

inline bool SaveGoldenHashes(const std::string& filePath, const std::map<std::string, uint64_t>& hashes) {
    std::ofstream file(filePath, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file << "# Software renderer golden framebuffer hashes\n";
    file << "# Update with RETRO_UPDATE_GOLDENS=1 when expected visuals intentionally change.\n";
    for (const auto& [key, value] : hashes) {
        file << key << "=" << value << "\n";
    }
    return file.good();
}

} // namespace RetroRenderer::TestGolden

