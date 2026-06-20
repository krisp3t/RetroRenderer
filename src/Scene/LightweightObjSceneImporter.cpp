#include "LightweightObjSceneImporter.h"

#include <KrisLogger/Logger.h>
#include <glm/geometric.hpp>
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
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

struct ObjPosition {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    bool hasColor = false;
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
    const auto [ptr, ec] = std::from_chars(begin, end, outValue);
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
    const size_t secondSlash =
        firstSlash == std::string_view::npos ? std::string_view::npos : token.find('/', firstSlash + 1);

    const std::string_view positionPart =
        firstSlash == std::string_view::npos ? token : token.substr(0, firstSlash);
    int rawPosition = 0;
    if (!ParseInteger(positionPart, rawPosition) ||
        !ResolveObjIndex(rawPosition, positionCount, outRef.positionIndex)) {
        return false;
    }

    if (firstSlash == std::string_view::npos) {
        return true;
    }

    const std::string_view texCoordPart = secondSlash == std::string_view::npos
                                              ? token.substr(firstSlash + 1)
                                              : token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
    if (!texCoordPart.empty()) {
        int rawTexCoord = 0;
        if (!ParseInteger(texCoordPart, rawTexCoord) ||
            !ResolveObjIndex(rawTexCoord, texCoordCount, outRef.texCoordIndex)) {
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
        if (!ParseInteger(normalPart, rawNormal) ||
            !ResolveObjIndex(rawNormal, normalCount, outRef.normalIndex)) {
            return false;
        }
        outRef.hasNormal = true;
    }

    return true;
}

glm::vec3 ComputeFaceNormal(const std::vector<FaceVertexRef>& faceRefs, const std::vector<ObjPosition>& positions) {
    glm::vec3 faceNormal(0.0f);
    if (faceRefs.size() < 3) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    for (size_t i = 0; i < faceRefs.size(); i++) {
        const glm::vec3& current = positions[faceRefs[i].positionIndex].position;
        const glm::vec3& next = positions[faceRefs[(i + 1) % faceRefs.size()].positionIndex].position;
        faceNormal.x += (current.y - next.y) * (current.z + next.z);
        faceNormal.y += (current.z - next.z) * (current.x + next.x);
        faceNormal.z += (current.x - next.x) * (current.y + next.y);
    }

    const float normalLen = glm::length(faceNormal);
    if (normalLen > 1e-6f) {
        return faceNormal / normalLen;
    }
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

std::string ReadTextFile(const std::filesystem::path& path) {
#ifdef __ANDROID__
    AAsset* asset = AAssetManager_open(g_assetManager, path.generic_string().c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        return {};
    }
    const size_t fileSize = AAsset_getLength(asset);
    std::string source(fileSize, '\0');
    AAsset_read(asset, source.data(), fileSize);
    AAsset_close(asset);
    return source;
#else
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
#endif
}

bool ParseMaterialLibrary(const std::filesystem::path& path,
                          ImportedSceneData& outSceneData,
                          std::unordered_map<std::string, int>& materialLookup) {
    const std::string source = ReadTextFile(path);
    if (source.empty()) {
        LOGW("OBJ importer: failed to open material library '%s'", path.string().c_str());
        return false;
    }

    int currentMaterialIndex = -1;
    size_t cursor = 0;
    size_t lineNumber = 0;
    while (cursor <= source.size()) {
        const size_t lineEnd = source.find('\n', cursor);
        std::string_view line =
            lineEnd == std::string_view::npos ? std::string_view(source).substr(cursor)
                                              : std::string_view(source).substr(cursor, lineEnd - cursor);
        cursor = lineEnd == std::string_view::npos ? source.size() + 1 : lineEnd + 1;
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
        const std::string_view payload =
            separator == std::string_view::npos ? std::string_view{} : Trim(line.substr(separator + 1));

        if (keyword == "newmtl") {
            ImportedMaterial material{};
            material.name = std::string(payload);
            currentMaterialIndex = static_cast<int>(outSceneData.materials.size());
            materialLookup[material.name] = currentMaterialIndex;
            outSceneData.materials.push_back(std::move(material));
            continue;
        }

        if (currentMaterialIndex < 0 || currentMaterialIndex >= static_cast<int>(outSceneData.materials.size())) {
            continue;
        }

        ImportedMaterial& material = outSceneData.materials[static_cast<size_t>(currentMaterialIndex)];
        if (keyword == "Kd") {
            std::istringstream stream{std::string(payload)};
            if (!(stream >> material.diffuseColor.r >> material.diffuseColor.g >> material.diffuseColor.b)) {
                LOGW("OBJ importer: invalid material diffuse color at line %zu", lineNumber);
            }
        } else if (keyword == "Ks") {
            std::istringstream stream{std::string(payload)};
            if (!(stream >> material.specularColor.r >> material.specularColor.g >> material.specularColor.b)) {
                LOGW("OBJ importer: invalid material specular color at line %zu", lineNumber);
            }
        } else if (keyword == "Ns") {
            std::istringstream stream{std::string(payload)};
            if (!(stream >> material.shininess)) {
                LOGW("OBJ importer: invalid material shininess at line %zu", lineNumber);
            }
        } else if (keyword == "d") {
            std::istringstream stream{std::string(payload)};
            if (!(stream >> material.alpha)) {
                LOGW("OBJ importer: invalid material alpha at line %zu", lineNumber);
            }
        } else if (keyword == "Tr") {
            float transparency = 0.0f;
            std::istringstream stream{std::string(payload)};
            if (!(stream >> transparency)) {
                LOGW("OBJ importer: invalid material transparency at line %zu", lineNumber);
            } else {
                material.alpha = 1.0f - transparency;
            }
        } else if (keyword == "map_Kd") {
            material.diffuseTexturePath = std::string(payload);
        }
    }

    return true;
}

int BeginObjectNode(ImportedSceneData& outSceneData, std::string_view objectName) {
    ImportedNode node{};
    node.name = objectName.empty() ? "Imported OBJ object" : std::string(objectName);
    const int nodeIndex = static_cast<int>(outSceneData.nodes.size());
    outSceneData.nodes.push_back(std::move(node));
    outSceneData.nodes[static_cast<size_t>(outSceneData.rootNodeIndex)].childNodeIndices.push_back(nodeIndex);
    return nodeIndex;
}

void FlushCurrentMesh(ImportedSceneData& outSceneData,
                      ImportedMesh& currentMesh,
                      const std::optional<int>& currentMaterialIndex,
                      int currentNodeIndex) {
    if (currentMesh.vertices.empty() || currentMesh.indices.empty()) {
        return;
    }

    currentMesh.materialIndex = currentMaterialIndex;
    const int meshIndex = static_cast<int>(outSceneData.meshes.size());
    outSceneData.meshes.push_back(std::move(currentMesh));
    outSceneData.nodes[static_cast<size_t>(currentNodeIndex)].meshIndices.push_back(meshIndex);
    currentMesh = ImportedMesh{};
}

bool ParseObjText(std::string_view sourceText,
                  std::string_view rootNodeName,
                  const std::filesystem::path& sourcePath,
                  ImportedSceneData& outSceneData) {
    std::vector<ObjPosition> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    ImportedMesh currentMesh{};
    std::optional<int> currentMaterialIndex;
    std::unordered_map<std::string, int> materialLookup;

    outSceneData = ImportedSceneData{};
    outSceneData.rootNodeIndex = 0;
    outSceneData.sourceDirectory = sourcePath.empty() ? "" : sourcePath.parent_path().string();
    ImportedNode rootNode{};
    rootNode.name = rootNodeName.empty() ? "Imported OBJ" : std::string(rootNodeName);
    outSceneData.nodes.push_back(std::move(rootNode));

    int currentNodeIndex = 0;
    size_t cursor = 0;
    size_t lineNumber = 0;
    while (cursor <= sourceText.size()) {
        const size_t lineEnd = sourceText.find('\n', cursor);
        std::string_view line =
            lineEnd == std::string_view::npos ? sourceText.substr(cursor) : sourceText.substr(cursor, lineEnd - cursor);
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
        const std::string_view payload =
            separator == std::string_view::npos ? std::string_view{} : Trim(line.substr(separator + 1));

        if (keyword == "mtllib") {
            if (sourcePath.empty()) {
                continue;
            }
            ParseMaterialLibrary(sourcePath.parent_path() / std::string(payload), outSceneData, materialLookup);
            continue;
        }

        if (keyword == "usemtl") {
            const auto it = materialLookup.find(std::string(payload));
            const std::optional<int> nextMaterialIndex =
                it == materialLookup.end() ? std::nullopt : std::optional<int>(it->second);
            if (currentMaterialIndex != nextMaterialIndex) {
                FlushCurrentMesh(outSceneData, currentMesh, currentMaterialIndex, currentNodeIndex);
                currentMaterialIndex = nextMaterialIndex;
            }
            if (it == materialLookup.end() && !payload.empty()) {
                LOGW("OBJ importer: unknown material '%s' at line %zu", std::string(payload).c_str(), lineNumber);
            }
            continue;
        }

        if (keyword == "o" || keyword == "g") {
            if (payload.empty()) {
                continue;
            }
            FlushCurrentMesh(outSceneData, currentMesh, currentMaterialIndex, currentNodeIndex);
            currentNodeIndex = BeginObjectNode(outSceneData, payload);
            continue;
        }

        if (keyword == "v") {
            std::istringstream stream{std::string(payload)};
            ObjPosition vertex{};
            if (!(stream >> vertex.position.x >> vertex.position.y >> vertex.position.z)) {
                LOGW("OBJ importer: invalid vertex position at line %zu", lineNumber);
                continue;
            }
            if (stream >> vertex.color.r >> vertex.color.g >> vertex.color.b) {
                vertex.hasColor = true;
            }
            positions.push_back(vertex);
            continue;
        }

        if (keyword == "vn") {
            std::istringstream stream{std::string(payload)};
            glm::vec3 normal(0.0f);
            if (!(stream >> normal.x >> normal.y >> normal.z)) {
                LOGW("OBJ importer: invalid normal at line %zu", lineNumber);
                continue;
            }
            normals.push_back(normal);
            continue;
        }

        if (keyword == "vt") {
            std::istringstream stream{std::string(payload)};
            glm::vec2 texCoord(0.0f);
            if (!(stream >> texCoord.x >> texCoord.y)) {
                LOGW("OBJ importer: invalid texcoord at line %zu", lineNumber);
                continue;
            }
            texCoords.push_back(texCoord);
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

        const bool needsComputedFaceNormal = std::any_of(faceRefs.begin(), faceRefs.end(), [](const FaceVertexRef& ref) {
            return !ref.hasNormal;
        });
        const glm::vec3 computedFaceNormal =
            needsComputedFaceNormal ? ComputeFaceNormal(faceRefs, positions) : glm::vec3(0.0f, 1.0f, 0.0f);

        for (size_t i = 1; i + 1 < faceRefs.size(); i++) {
            const FaceVertexRef triRefs[3] = {faceRefs[0], faceRefs[i], faceRefs[i + 1]};
            for (const FaceVertexRef& ref : triRefs) {
                Vertex vertex{};
                const ObjPosition& sourcePosition = positions[static_cast<size_t>(ref.positionIndex)];
                vertex.position = glm::vec4(sourcePosition.position, 1.0f);
                vertex.texCoords = ref.hasTexCoord ? texCoords[static_cast<size_t>(ref.texCoordIndex)] : glm::vec2(0.0f);
                vertex.normal = ref.hasNormal ? normals[static_cast<size_t>(ref.normalIndex)] : computedFaceNormal;
                vertex.color = sourcePosition.hasColor ? sourcePosition.color : glm::vec3(1.0f);

                currentMesh.indices.push_back(static_cast<unsigned int>(currentMesh.vertices.size()));
                currentMesh.vertices.push_back(vertex);
            }
        }
    }

    FlushCurrentMesh(outSceneData, currentMesh, currentMaterialIndex, currentNodeIndex);

    if (outSceneData.meshes.empty()) {
        LOGE("OBJ importer: no renderable triangles found");
        return false;
    }
    return true;
}

} // namespace

bool LightweightObjSceneImporter::LoadFromMemory(const uint8_t* data, size_t size, ImportedSceneData& outSceneData) {
    if (!data || size == 0) {
        LOGE("OBJ importer: empty memory buffer");
        return false;
    }
    const std::string_view source(reinterpret_cast<const char*>(data), size);
    return ParseObjText(source, "Imported OBJ", {}, outSceneData);
}

bool LightweightObjSceneImporter::LoadFromFile(const std::string& path, ImportedSceneData& outSceneData) {
    const std::filesystem::path sourcePath(path);
    const std::string rootNodeName = sourcePath.filename().string();
    const std::string source = ReadTextFile(sourcePath);
    if (source.empty()) {
        LOGE("OBJ importer: failed to open file '%s'", path.c_str());
        return false;
    }
    return ParseObjText(source, rootNodeName, sourcePath, outSceneData);
}

} // namespace RetroRenderer
