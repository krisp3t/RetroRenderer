#include "LightweightObjSceneImporter.h"

#include <KrisLogger/Logger.h>
#include <glm/geometric.hpp>
#include <charconv>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifdef __ANDROID__
#include "../native/AndroidBridge.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

namespace RetroRenderer {
namespace {
struct FaceVertexRef {
    int positionIndex = -1;
    int texCoordIndex = -1;
    int normalIndex = -1;
    bool hasTexCoord = false;
    bool hasNormal = false;
};

std::string_view Trim(std::string_view value) {
    size_t begin = 0;
    while (begin < value.size() && (value[begin] == ' ' || value[begin] == '\t')) {
        begin++;
    }
    size_t end = value.size();
    while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t')) {
        end--;
    }
    return value.substr(begin, end - begin);
}

bool ParseInteger(std::string_view token, int& outValue) {
    if (token.empty()) {
        return false;
    }
    const char* begin = token.data();
    const char* end = token.data() + token.size();
    auto [ptr, ec] = std::from_chars(begin, end, outValue);
    return ec == std::errc{} && ptr == end;
}

bool ResolveObjIndex(int rawIndex, size_t count, int& outIndex) {
    if (count == 0 || rawIndex == 0) {
        return false;
    }
    const int countInt = static_cast<int>(count);
    if (rawIndex > 0) {
        outIndex = rawIndex - 1;
    } else {
        outIndex = countInt + rawIndex;
    }
    return outIndex >= 0 && outIndex < countInt;
}

bool ParseFaceVertexToken(std::string_view token,
                          size_t positionCount,
                          size_t texCoordCount,
                          size_t normalCount,
                          FaceVertexRef& outRef) {
    const size_t firstSlash = token.find('/');
    const size_t secondSlash = firstSlash == std::string_view::npos ? std::string_view::npos : token.find('/', firstSlash + 1);

    const std::string_view positionPart = firstSlash == std::string_view::npos ? token : token.substr(0, firstSlash);
    int rawPosition = 0;
    if (!ParseInteger(positionPart, rawPosition) || !ResolveObjIndex(rawPosition, positionCount, outRef.positionIndex)) {
        return false;
    }

    if (firstSlash == std::string_view::npos) {
        return true;
    }

    const std::string_view texCoordPart =
        secondSlash == std::string_view::npos ? token.substr(firstSlash + 1) : token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
    if (!texCoordPart.empty()) {
        int rawTexCoord = 0;
        if (!ParseInteger(texCoordPart, rawTexCoord) || !ResolveObjIndex(rawTexCoord, texCoordCount, outRef.texCoordIndex)) {
            return false;
        }
        outRef.hasTexCoord = true;
    }

    if (secondSlash == std::string_view::npos) {
        return true;
    }


    const std::string_view normalPart = token.substr(secondSlash + 1);
    if (!normalPart.empty()) {
        int rawNormal = 0;
        if (!ParseInteger(normalPart, rawNormal) || !ResolveObjIndex(rawNormal, normalCount, outRef.normalIndex)) {
            return false;
        }
        outRef.hasNormal = true;
    }

    return true;
}

bool ParseObjText(std::string_view sourceText, ImportedSceneData& outSceneData) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    ImportedMesh mesh{};

    size_t cursor = 0;
    size_t lineNumber = 0;
    while (cursor <= sourceText.size()) {
        const size_t lineEnd = sourceText.find('\n', cursor);
        std::string_view line = lineEnd == std::string_view::npos ? sourceText.substr(cursor) : sourceText.substr(cursor, lineEnd - cursor);
        cursor = lineEnd == std::string_view::npos ? sourceText.size() + 1 : lineEnd + 1;
        lineNumber++;

        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        const size_t commentPos = line.find('#');
        if (commentPos != std::string_view::npos) {
            line = line.substr(0, commentPos);
        }
        line = Trim(line);
        if (line.empty()) {
            continue;
        }

        const size_t separator = line.find_first_of(" \t");
        const std::string_view keyword = separator == std::string_view::npos ? line : line.substr(0, separator);
        const std::string_view payload = separator == std::string_view::npos ? std::string_view{} : Trim(line.substr(separator + 1));

        if (keyword == "v") {
            std::istringstream stream{std::string(payload)};
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (!(stream >> x >> y >> z)) {
                LOGW("OBJ importer: invalid vertex position at line %zu", lineNumber);
                continue;
            }
            positions.emplace_back(x, y, z);
            continue;
        }

        if (keyword == "vn") {
            std::istringstream stream{std::string(payload)};
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (!(stream >> x >> y >> z)) {
                LOGW("OBJ importer: invalid normal at line %zu", lineNumber);
                continue;
            }
            normals.emplace_back(x, y, z);
            continue;
        }

        if (keyword == "vt") {
            std::istringstream stream{std::string(payload)};
            float u = 0.0f;
            float v = 0.0f;
            if (!(stream >> u >> v)) {
                LOGW("OBJ importer: invalid texcoord at line %zu", lineNumber);
                continue;
            }
            texCoords.emplace_back(u, v);
            continue;
        }

        if (keyword != "f") {
            continue;
        }

        std::istringstream stream{std::string(payload)};
        std::string token;
        std::vector<FaceVertexRef> faceRefs;
        while (stream >> token) {
            FaceVertexRef ref{};
            if (!ParseFaceVertexToken(token, positions.size(), texCoords.size(), normals.size(), ref)) {
                LOGW("OBJ importer: invalid face token '%s' at line %zu", token.c_str(), lineNumber);
                faceRefs.clear();
                break;
            }
            faceRefs.push_back(ref);
        }
        if (faceRefs.size() < 3) {
            continue;
        }

        for (size_t i = 1; i + 1 < faceRefs.size(); i++) {
            const FaceVertexRef triRefs[3] = {faceRefs[0], faceRefs[i], faceRefs[i + 1]};
            const glm::vec3 triPositions[3] = {positions[triRefs[0].positionIndex],
                                               positions[triRefs[1].positionIndex],
                                               positions[triRefs[2].positionIndex]};
            glm::vec3 computedNormal = glm::cross(triPositions[1] - triPositions[0], triPositions[2] - triPositions[0]);
            const float normalLen = glm::length(computedNormal);
            if (normalLen > 1e-6f) {
                computedNormal /= normalLen;
            } else {
                computedNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            for (const FaceVertexRef& ref : triRefs) {
                Vertex vertex{};
                vertex.position = glm::vec4(positions[ref.positionIndex], 1.0f);
                vertex.texCoords = ref.hasTexCoord ? texCoords[ref.texCoordIndex] : glm::vec2(0.0f, 0.0f);
                vertex.normal = ref.hasNormal ? normals[ref.normalIndex] : computedNormal;
                vertex.color = glm::vec3(1.0f);

                mesh.indices.push_back(static_cast<unsigned int>(mesh.vertices.size()));
                mesh.vertices.push_back(vertex);
            }
        }
    }

    if (mesh.vertices.empty() || mesh.indices.empty()) {
        LOGE("OBJ importer: no renderable triangles found");
        return false;
    }

    ImportedNode rootNode{};
    rootNode.name = "OBJRoot";
    rootNode.meshIndices.push_back(0);

    outSceneData = ImportedSceneData{};
    outSceneData.meshes.push_back(std::move(mesh));
    outSceneData.nodes.push_back(std::move(rootNode));
    outSceneData.rootNodeIndex = 0;
    return true;
}
} // namespace

bool LightweightObjSceneImporter::LoadFromMemory(const uint8_t* data, size_t size, ImportedSceneData& outSceneData) {
    if (!data || size == 0) {
        LOGE("OBJ importer: empty memory buffer");
        return false;
    }
    const std::string_view source(reinterpret_cast<const char*>(data), size);
    return ParseObjText(source, outSceneData);
}

bool LightweightObjSceneImporter::LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) {
#ifdef __ANDROID__
    AAsset* asset = AAssetManager_open(g_assetManager, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("OBJ importer: failed to open asset '%s'", path.c_str());
        return false;
    }
    const size_t fileSize = AAsset_getLength(asset);
    std::string source(fileSize, '\0');
    AAsset_read(asset, source.data(), fileSize);
    AAsset_close(asset);
    return ParseObjText(source, outSceneData);
#else
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOGE("OBJ importer: failed to open file '%s'", path.c_str());
        return false;
    }
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return ParseObjText(source, outSceneData);
#endif
}

} // namespace RetroRenderer
