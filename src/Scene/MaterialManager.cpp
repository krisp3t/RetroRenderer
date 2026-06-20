#include "MaterialManager.h"

#include "../Renderer/MaterialTypes.h"
#include "../Renderer/RenderServices.h"
#include "Scene.h"
#include <KrisLogger/Logger.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

#ifdef __ANDROID__
#include "../native/android/AndroidBridge.h"
#include <android/asset_manager.h>
#endif

namespace RetroRenderer {
namespace {
using json = nlohmann::json;

struct MaterialTemplateSource {
    struct Node {
        std::string id;
        MaterialOpcode opcode = MaterialOpcode::CONSTANT;
        MaterialDataType resultType = MaterialDataType::VEC4;
        std::vector<std::string> inputs;
        glm::vec4 constantValue = glm::vec4(0.0f);
        std::string parameterName;
        std::string samplerName;
        std::string semanticName;
        std::string swizzlePattern;
    };

    struct Stage {
        std::vector<Node> nodes;
        std::unordered_map<std::string, std::string> outputs;
    };

    std::string name;
    MaterialPipelineState pipelineState{};
    std::vector<MaterialParameterDesc> parameters;
    std::vector<MaterialSamplerDesc> samplers;
    Stage vertex;
    Stage fragment;
};

std::string ToLower(std::string_view value) {
    std::string lowered(value);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lowered;
}

int ComponentCount(MaterialDataType type) {
    switch (type) {
    case MaterialDataType::FLOAT1:
    case MaterialDataType::BOOL1:
        return 1;
    case MaterialDataType::VEC2:
        return 2;
    case MaterialDataType::VEC3:
        return 3;
    case MaterialDataType::VEC4:
        return 4;
    }
    return 4;
}

glm::vec4 PadToVec4(const glm::vec4& value, MaterialDataType type) {
    switch (type) {
    case MaterialDataType::FLOAT1:
    case MaterialDataType::BOOL1:
        return glm::vec4(value.x, 0.0f, 0.0f, 0.0f);
    case MaterialDataType::VEC2:
        return glm::vec4(value.x, value.y, 0.0f, 0.0f);
    case MaterialDataType::VEC3:
        return glm::vec4(value.x, value.y, value.z, 0.0f);
    case MaterialDataType::VEC4:
        return value;
    }
    return value;
}

std::filesystem::path AssetsRootPath() {
#ifdef __ANDROID__
    return std::filesystem::path(g_assetsPath);
#else
    return std::filesystem::path("assets");
#endif
}

MaterialDataType ParseMaterialDataType(const std::string& text, MaterialDataType fallback = MaterialDataType::VEC4) {
    const std::string lowered = ToLower(text);
    if (lowered == "float" || lowered == "float1") {
        return MaterialDataType::FLOAT1;
    }
    if (lowered == "vec2") {
        return MaterialDataType::VEC2;
    }
    if (lowered == "vec3") {
        return MaterialDataType::VEC3;
    }
    if (lowered == "vec4") {
        return MaterialDataType::VEC4;
    }
    if (lowered == "bool" || lowered == "bool1") {
        return MaterialDataType::BOOL1;
    }
    return fallback;
}

MaterialShadingModel ParseShadingModel(const std::string& text) {
    const std::string lowered = ToLower(text);
    if (lowered == "unlit") {
        return MaterialShadingModel::UNLIT;
    }
    if (lowered == "lambert") {
        return MaterialShadingModel::LAMBERT;
    }
    return MaterialShadingModel::PHONG;
}

MaterialBlendMode ParseBlendMode(const std::string& text) {
    const std::string lowered = ToLower(text);
    if (lowered == "alpha_blend" || lowered == "alpha-blend" || lowered == "blend") {
        return MaterialBlendMode::ALPHA_BLEND;
    }
    if (lowered == "alpha_cutout" || lowered == "alpha-cutout" || lowered == "cutout" || lowered == "masked") {
        return MaterialBlendMode::ALPHA_CUTOUT;
    }
    return MaterialBlendMode::OPAQUE;
}

MaterialCullMode ParseCullMode(const std::string& text) {
    const std::string lowered = ToLower(text);
    if (lowered == "none" || lowered == "off") {
        return MaterialCullMode::NONE;
    }
    if (lowered == "front") {
        return MaterialCullMode::FRONT;
    }
    return MaterialCullMode::BACK;
}

MaterialFilterMode ParseFilterMode(const std::string& text) {
    return ToLower(text) == "nearest" ? MaterialFilterMode::NEAREST : MaterialFilterMode::LINEAR;
}

MaterialWrapMode ParseWrapMode(const std::string& text) {
    return ToLower(text) == "clamp_to_edge" || ToLower(text) == "clamp" ? MaterialWrapMode::CLAMP_TO_EDGE
                                                                           : MaterialWrapMode::REPEAT;
}

MaterialSemantic ParseSemantic(const std::string& text) {
    const std::string lowered = ToLower(text);
    if (lowered == "positionos" || lowered == "position_os") {
        return MaterialSemantic::POSITION_OS;
    }
    if (lowered == "normalos" || lowered == "normal_os") {
        return MaterialSemantic::NORMAL_OS;
    }
    if (lowered == "uv0") {
        return MaterialSemantic::UV0;
    }
    if (lowered == "color0") {
        return MaterialSemantic::COLOR0;
    }
    if (lowered == "time") {
        return MaterialSemantic::TIME;
    }
    if (lowered == "worldposition" || lowered == "world_position") {
        return MaterialSemantic::WORLD_POSITION;
    }
    if (lowered == "normalws" || lowered == "normal_ws") {
        return MaterialSemantic::NORMAL_WS;
    }
    if (lowered == "viewdirws" || lowered == "view_dir_ws") {
        return MaterialSemantic::VIEW_DIR_WS;
    }
    if (lowered == "screenuv" || lowered == "screen_uv") {
        return MaterialSemantic::SCREEN_UV;
    }
    if (lowered == "varying0") {
        return MaterialSemantic::VARYING0;
    }
    if (lowered == "varying1") {
        return MaterialSemantic::VARYING1;
    }
    if (lowered == "varying2") {
        return MaterialSemantic::VARYING2;
    }
    return MaterialSemantic::VARYING3;
}

MaterialOpcode ParseOpcode(const std::string& text) {
    const std::string lowered = ToLower(text);
    if (lowered == "constant") {
        return MaterialOpcode::CONSTANT;
    }
    if (lowered == "parameter") {
        return MaterialOpcode::PARAMETER;
    }
    if (lowered == "semantic") {
        return MaterialOpcode::SEMANTIC;
    }
    if (lowered == "add") {
        return MaterialOpcode::ADD;
    }
    if (lowered == "subtract") {
        return MaterialOpcode::SUBTRACT;
    }
    if (lowered == "multiply") {
        return MaterialOpcode::MULTIPLY;
    }
    if (lowered == "divide") {
        return MaterialOpcode::DIVIDE;
    }
    if (lowered == "min" || lowered == "minimum") {
        return MaterialOpcode::MINIMUM;
    }
    if (lowered == "max" || lowered == "maximum") {
        return MaterialOpcode::MAXIMUM;
    }
    if (lowered == "clamp") {
        return MaterialOpcode::CLAMP;
    }
    if (lowered == "saturate") {
        return MaterialOpcode::SATURATE;
    }
    if (lowered == "abs" || lowered == "absolute") {
        return MaterialOpcode::ABSOLUTE;
    }
    if (lowered == "floor") {
        return MaterialOpcode::FLOOR;
    }
    if (lowered == "frac" || lowered == "fraction") {
        return MaterialOpcode::FRACTION;
    }
    if (lowered == "sin" || lowered == "sine") {
        return MaterialOpcode::SINE;
    }
    if (lowered == "cos" || lowered == "cosine") {
        return MaterialOpcode::COSINE;
    }
    if (lowered == "dot") {
        return MaterialOpcode::DOT;
    }
    if (lowered == "normalize") {
        return MaterialOpcode::NORMALIZE;
    }
    if (lowered == "length") {
        return MaterialOpcode::LENGTH;
    }
    if (lowered == "pow" || lowered == "power") {
        return MaterialOpcode::POWER;
    }
    if (lowered == "lerp" || lowered == "mix") {
        return MaterialOpcode::LERP;
    }
    if (lowered == "append") {
        return MaterialOpcode::APPEND;
    }
    if (lowered == "swizzle") {
        return MaterialOpcode::SWIZZLE;
    }
    return MaterialOpcode::SAMPLE_TEXTURE;
}

glm::vec4 ParseJsonValue(const json& value, MaterialDataType type) {
    if (value.is_boolean()) {
        return glm::vec4(value.get<bool>() ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
    }
    if (value.is_number()) {
        return glm::vec4(value.get<float>(), 0.0f, 0.0f, 0.0f);
    }

    glm::vec4 result(0.0f);
    if (value.is_array()) {
        const int count = std::min<int>(static_cast<int>(value.size()), 4);
        for (int i = 0; i < count; i++) {
            result[i] = value[static_cast<size_t>(i)].get<float>();
        }
    }
    return PadToVec4(result, type);
}

std::string FormatFloatLiteral(float value) {
    std::ostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(6);
    stream << value;
    return stream.str();
}

std::string GlslValueLiteral(const glm::vec4& value, MaterialDataType type) {
    switch (type) {
    case MaterialDataType::FLOAT1:
        return FormatFloatLiteral(value.x);
    case MaterialDataType::BOOL1:
        return value.x >= 0.5f ? "true" : "false";
    case MaterialDataType::VEC2:
        return "vec2(" + FormatFloatLiteral(value.x) + ", " + FormatFloatLiteral(value.y) + ")";
    case MaterialDataType::VEC3:
        return "vec3(" + FormatFloatLiteral(value.x) + ", " + FormatFloatLiteral(value.y) + ", " + FormatFloatLiteral(value.z) + ")";
    case MaterialDataType::VEC4:
        return "vec4(" + FormatFloatLiteral(value.x) + ", " + FormatFloatLiteral(value.y) + ", " + FormatFloatLiteral(value.z) + ", " +
               FormatFloatLiteral(value.w) + ")";
    }
    return "vec4(0.0)";
}

std::string WrapGlslRegisterValue(const std::string& expression, MaterialDataType type) {
    switch (type) {
    case MaterialDataType::FLOAT1:
        return "vec4(" + expression + ", 0.0, 0.0, 0.0)";
    case MaterialDataType::BOOL1:
        return "vec4((" + expression + ") ? 1.0 : 0.0, 0.0, 0.0, 0.0)";
    case MaterialDataType::VEC2:
        return "vec4(" + expression + ", 0.0, 0.0)";
    case MaterialDataType::VEC3:
        return "vec4(" + expression + ", 0.0)";
    case MaterialDataType::VEC4:
        return expression;
    }
    return expression;
}

std::string ReadRegisterExpression(const MaterialStageProgram& stage, int registerIndex) {
    const MaterialDataType type = stage.registerTypes[static_cast<size_t>(registerIndex)];
    const std::string base = "r" + std::to_string(registerIndex);
    switch (type) {
    case MaterialDataType::FLOAT1:
    case MaterialDataType::BOOL1:
        return base + ".x";
    case MaterialDataType::VEC2:
        return base + ".xy";
    case MaterialDataType::VEC3:
        return base + ".xyz";
    case MaterialDataType::VEC4:
        return base;
    }
    return base;
}

std::string VertexSemanticExpression(MaterialSemantic semantic) {
    switch (semantic) {
    case MaterialSemantic::POSITION_OS:
        return "a_Position";
    case MaterialSemantic::NORMAL_OS:
        return "a_Normal";
    case MaterialSemantic::UV0:
        return "a_TexCoord";
    case MaterialSemantic::COLOR0:
        return "a_Color";
    case MaterialSemantic::TIME:
        return "u_Time";
    default:
        break;
    }
    return "vec4(0.0)";
}

std::string FragmentSemanticExpression(MaterialSemantic semantic) {
    switch (semantic) {
    case MaterialSemantic::WORLD_POSITION:
        return "v_WorldPos";
    case MaterialSemantic::NORMAL_WS:
        return "normalize(v_NormalWS)";
    case MaterialSemantic::UV0:
        return "v_TexCoord";
    case MaterialSemantic::COLOR0:
        return "v_Color";
    case MaterialSemantic::VIEW_DIR_WS:
        return "normalize(u_ViewPos - v_WorldPos)";
    case MaterialSemantic::SCREEN_UV:
        return "gl_FragCoord.xy / max(u_FrameSize, vec2(1.0))";
    case MaterialSemantic::VARYING0:
        return "v_Varying0";
    case MaterialSemantic::VARYING1:
        return "v_Varying1";
    case MaterialSemantic::VARYING2:
        return "v_Varying2";
    case MaterialSemantic::VARYING3:
        return "v_Varying3";
    default:
        break;
    }
    return "vec4(0.0)";
}

std::string SwizzleSuffix(const MaterialInstruction& instruction) {
    std::string suffix(".");
    for (int i = 0; i < instruction.componentCount; i++) {
        static constexpr char kComponents[] = {'x', 'y', 'z', 'w'};
        suffix.push_back(kComponents[instruction.swizzle[static_cast<size_t>(i)]]);
    }
    return suffix;
}

uint64_t HashSourceText(const std::string& text) {
    return static_cast<uint64_t>(std::hash<std::string>{}(text));
}

std::string ResolveVertexOutputExpression(const MaterialStageProgram& stage, int registerIndex, const char* fallback) {
    if (registerIndex < 0) {
        return fallback;
    }
    return ReadRegisterExpression(stage, registerIndex);
}

std::string ResolveFragmentOutputExpression(const MaterialStageProgram& stage, int registerIndex, MaterialDataType type, const char* fallback) {
    if (registerIndex < 0) {
        return fallback;
    }
    (void)type;
    return ReadRegisterExpression(stage, registerIndex);
}

bool ParseTemplateStage(const json& stageJson, MaterialTemplateSource::Stage& outStage, const char* stageName) {
    outStage = MaterialTemplateSource::Stage{};
    if (!stageJson.is_object()) {
        LOGE("Material template %s stage must be an object", stageName);
        return false;
    }

    if (const auto opsIt = stageJson.find("ops"); opsIt != stageJson.end() && opsIt->is_array()) {
        outStage.nodes.reserve(opsIt->size());
        for (const json& opJson : *opsIt) {
            if (!opJson.is_object()) {
                continue;
            }
            MaterialTemplateSource::Node node{};
            node.id = opJson.value("id", "");
            node.opcode = ParseOpcode(opJson.value("op", "constant"));
            node.resultType = ParseMaterialDataType(opJson.value("type", "vec4"));
            if (const auto inputsIt = opJson.find("inputs"); inputsIt != opJson.end() && inputsIt->is_array()) {
                for (const json& inputValue : *inputsIt) {
                    node.inputs.push_back(inputValue.get<std::string>());
                }
            }
            if (const auto inputIt = opJson.find("input"); inputIt != opJson.end() && inputIt->is_string()) {
                node.inputs.push_back(inputIt->get<std::string>());
            }
            if (const auto uvIt = opJson.find("uv"); uvIt != opJson.end() && uvIt->is_string()) {
                node.inputs.push_back(uvIt->get<std::string>());
            }
            node.parameterName = opJson.value("parameter", "");
            node.samplerName = opJson.value("sampler", "");
            node.semanticName = opJson.value("semantic", "");
            node.swizzlePattern = opJson.value("pattern", "");
            if (const auto valueIt = opJson.find("value"); valueIt != opJson.end()) {
                node.constantValue = ParseJsonValue(*valueIt, node.resultType);
            }
            if (node.id.empty()) {
                LOGE("Material template %s stage contains a node without an id", stageName);
                return false;
            }
            outStage.nodes.push_back(std::move(node));
        }
    }

    if (const auto outputsIt = stageJson.find("outputs"); outputsIt != stageJson.end() && outputsIt->is_object()) {
        for (const auto& [key, value] : outputsIt->items()) {
            if (value.is_string()) {
                outStage.outputs[key] = value.get<std::string>();
            }
        }
    }
    return true;
}

bool ParseTemplateSource(const std::filesystem::path& path, const std::string& sourceText, MaterialTemplateSource& outSource) {
    outSource = MaterialTemplateSource{};
    if (sourceText.empty()) {
        LOGE("Material template %s is empty", path.string().c_str());
        return false;
    }

    json root;
    try {
        root = json::parse(sourceText);
    } catch (const json::exception& exception) {
        LOGE("Failed to parse material template %s: %s", path.string().c_str(), exception.what());
        return false;
    }

    outSource.name = root.value("name", path.stem().string());

    if (const auto pipelineIt = root.find("pipeline"); pipelineIt != root.end() && pipelineIt->is_object()) {
        outSource.pipelineState.shadingModel = ParseShadingModel(pipelineIt->value("shadingModel", root.value("shadingModel", "phong")));
        outSource.pipelineState.blendMode = ParseBlendMode(pipelineIt->value("blendMode", "opaque"));
        outSource.pipelineState.cullMode = ParseCullMode(pipelineIt->value("cullMode", "back"));
        outSource.pipelineState.depthTest = pipelineIt->value("depthTest", true);
        outSource.pipelineState.depthWrite = pipelineIt->value("depthWrite", true);
        outSource.pipelineState.alphaCutoff = pipelineIt->value("alphaCutoff", 0.5f);
        outSource.pipelineState.boundsPadding = pipelineIt->value("boundsPadding", 0.0f);
    } else {
        outSource.pipelineState.shadingModel = ParseShadingModel(root.value("shadingModel", "phong"));
    }

    if (const auto paramsIt = root.find("parameters"); paramsIt != root.end() && paramsIt->is_array()) {
        outSource.parameters.reserve(paramsIt->size());
        for (const json& paramJson : *paramsIt) {
            if (!paramJson.is_object()) {
                continue;
            }
            MaterialParameterDesc parameter{};
            parameter.name = paramJson.value("name", "");
            parameter.type = ParseMaterialDataType(paramJson.value("type", "vec4"));
            parameter.defaultValue = MaterialValue{
                .type = parameter.type,
                .data = ParseJsonValue(paramJson.contains("default") ? paramJson["default"] : json{}, parameter.type),
            };
            if (!parameter.name.empty()) {
                outSource.parameters.push_back(std::move(parameter));
            }
        }
    }

    if (const auto samplersIt = root.find("samplers"); samplersIt != root.end() && samplersIt->is_array()) {
        outSource.samplers.reserve(samplersIt->size());
        for (const json& samplerJson : *samplersIt) {
            if (!samplerJson.is_object()) {
                continue;
            }
            MaterialSamplerDesc sampler{};
            sampler.name = samplerJson.value("name", "");
            sampler.filter = ParseFilterMode(samplerJson.value("filter", "linear"));
            sampler.wrapU = ParseWrapMode(samplerJson.value("wrapU", samplerJson.value("wrap", "repeat")));
            sampler.wrapV = ParseWrapMode(samplerJson.value("wrapV", samplerJson.value("wrap", "repeat")));
            if (!sampler.name.empty()) {
                outSource.samplers.push_back(std::move(sampler));
            }
        }
    }

    if (!ParseTemplateStage(root.value("vertex", json::object()), outSource.vertex, "vertex")) {
        return false;
    }
    if (!ParseTemplateStage(root.value("fragment", json::object()), outSource.fragment, "fragment")) {
        return false;
    }

    return true;
}

int AppendInstruction(MaterialStageProgram& stage, std::vector<MaterialInstruction>& instructions, MaterialInstruction instruction) {
    instruction.dstRegister = stage.registerCount;
    stage.registerTypes.push_back(instruction.resultType);
    stage.registerCount++;
    instructions.push_back(std::move(instruction));
    return stage.registerCount - 1;
}

int AppendConstant(MaterialStageProgram& stage, std::vector<MaterialInstruction>& instructions, MaterialDataType type, const glm::vec4& value) {
    MaterialInstruction instruction{};
    instruction.opcode = MaterialOpcode::CONSTANT;
    instruction.resultType = type;
    instruction.immediate = PadToVec4(value, type);
    instruction.componentCount = static_cast<uint8_t>(ComponentCount(type));
    return AppendInstruction(stage, instructions, instruction);
}

bool CompileStageProgram(const MaterialTemplateSource& source,
                         const MaterialTemplateSource::Stage& stageSource,
                         bool vertexStage,
                         MaterialStageProgram& outProgram,
                         MaterialVertexOutputs* vertexOutputs,
                         MaterialFragmentOutputs* fragmentOutputs,
                         bool& outUsesVertexColor,
                         bool& outUsesTextureSampling,
                         int& outVaryingCount) {
    outProgram = MaterialStageProgram{};
    std::unordered_map<std::string, int> parameterIndices;
    std::unordered_map<std::string, int> samplerIndices;
    std::unordered_map<std::string, int> registerById;
    std::vector<MaterialInstruction> compiledInstructions;

    for (size_t i = 0; i < source.parameters.size(); i++) {
        parameterIndices.emplace(source.parameters[i].name, static_cast<int>(i));
    }
    for (size_t i = 0; i < source.samplers.size(); i++) {
        samplerIndices.emplace(source.samplers[i].name, static_cast<int>(i));
    }

    for (const MaterialTemplateSource::Node& node : stageSource.nodes) {
        MaterialInstruction instruction{};
        instruction.opcode = node.opcode;
        instruction.resultType = node.resultType;
        instruction.componentCount = static_cast<uint8_t>(ComponentCount(node.resultType));

        switch (node.opcode) {
        case MaterialOpcode::CONSTANT:
            instruction.immediate = node.constantValue;
            break;
        case MaterialOpcode::PARAMETER: {
            const auto it = parameterIndices.find(node.parameterName);
            if (it == parameterIndices.end()) {
                LOGE("Material template node %s references unknown parameter %s", node.id.c_str(), node.parameterName.c_str());
                return false;
            }
            instruction.parameterIndex = it->second;
            break;
        }
        case MaterialOpcode::SEMANTIC:
            instruction.semantic = ParseSemantic(node.semanticName);
            outUsesVertexColor |= instruction.semantic == MaterialSemantic::COLOR0;
            break;
        case MaterialOpcode::SAMPLE_TEXTURE: {
            const auto samplerIt = samplerIndices.find(node.samplerName);
            if (samplerIt == samplerIndices.end()) {
                LOGE("Material template node %s references unknown sampler %s", node.id.c_str(), node.samplerName.c_str());
                return false;
            }
            if (node.inputs.empty()) {
                LOGE("Material template node %s is missing uv input", node.id.c_str());
                return false;
            }
            const auto uvIt = registerById.find(node.inputs.front());
            if (uvIt == registerById.end()) {
                LOGE("Material template node %s references unknown uv register %s", node.id.c_str(), node.inputs.front().c_str());
                return false;
            }
            instruction.samplerIndex = samplerIt->second;
            instruction.srcRegisters[0] = uvIt->second;
            outUsesTextureSampling = true;
            break;
        }
        case MaterialOpcode::SWIZZLE:
            if (node.inputs.empty()) {
                LOGE("Material template swizzle node %s is missing an input", node.id.c_str());
                return false;
            }
            if (node.swizzlePattern.empty()) {
                LOGE("Material template swizzle node %s is missing a pattern", node.id.c_str());
                return false;
            }
            for (int i = 0; i < static_cast<int>(node.swizzlePattern.size()) && i < 4; i++) {
                const char c = static_cast<char>(std::tolower(node.swizzlePattern[static_cast<size_t>(i)]));
                instruction.swizzle[static_cast<size_t>(i)] = c == 'y' ? 1 : c == 'z' ? 2 : c == 'w' ? 3 : 0;
            }
            instruction.componentCount = static_cast<uint8_t>(std::min<int>(node.swizzlePattern.size(), 4));
            [[fallthrough]];
        default:
            for (size_t inputIndex = 0; inputIndex < node.inputs.size() && inputIndex < instruction.srcRegisters.size(); inputIndex++) {
                const auto regIt = registerById.find(node.inputs[inputIndex]);
                if (regIt == registerById.end()) {
                    LOGE("Material template node %s references unknown input %s", node.id.c_str(), node.inputs[inputIndex].c_str());
                    return false;
                }
                instruction.srcRegisters[inputIndex] = regIt->second;
            }
            break;
        }

        const int registerIndex = AppendInstruction(outProgram, compiledInstructions, instruction);
        registerById[node.id] = registerIndex;
    }

    auto resolveStageOutput = [&](const std::string& outputName, int& outputRegister, MaterialDataType type, const glm::vec4& fallback) {
        const auto it = stageSource.outputs.find(outputName);
        if (it != stageSource.outputs.end()) {
            const auto regIt = registerById.find(it->second);
            if (regIt != registerById.end()) {
                outputRegister = regIt->second;
                return;
            }
            LOGW("Material template output %s references unknown node %s; using default", outputName.c_str(), it->second.c_str());
        }
        outputRegister = AppendConstant(outProgram, compiledInstructions, type, fallback);
    };

    if (vertexStage) {
        resolveStageOutput("positionOS", vertexOutputs->positionRegister, MaterialDataType::VEC4, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        resolveStageOutput("normalOS", vertexOutputs->normalRegister, MaterialDataType::VEC3, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
        resolveStageOutput("uv0", vertexOutputs->uvRegister, MaterialDataType::VEC2, glm::vec4(0.0f));
        resolveStageOutput("color0", vertexOutputs->colorRegister, MaterialDataType::VEC4, glm::vec4(1.0f));
        for (int varyingIndex = 0; varyingIndex < 4; varyingIndex++) {
            const std::string outputName = "varying" + std::to_string(varyingIndex);
            const auto it = stageSource.outputs.find(outputName);
            if (it == stageSource.outputs.end()) {
                continue;
            }
            const auto regIt = registerById.find(it->second);
            if (regIt == registerById.end()) {
                continue;
            }
            vertexOutputs->varyingRegisters[static_cast<size_t>(varyingIndex)] = regIt->second;
            outVaryingCount = std::max(outVaryingCount, varyingIndex + 1);
        }
    } else {
        resolveStageOutput("baseColor", fragmentOutputs->baseColorRegister, MaterialDataType::VEC4, glm::vec4(1.0f));
        resolveStageOutput("emissive", fragmentOutputs->emissiveRegister, MaterialDataType::VEC3, glm::vec4(0.0f));
        resolveStageOutput("alpha", fragmentOutputs->alphaRegister, MaterialDataType::FLOAT1, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        resolveStageOutput("ambientStrength", fragmentOutputs->ambientStrengthRegister, MaterialDataType::FLOAT1, glm::vec4(0.3f, 0.0f, 0.0f, 0.0f));
        resolveStageOutput("specularStrength", fragmentOutputs->specularStrengthRegister, MaterialDataType::FLOAT1, glm::vec4(0.3f, 0.0f, 0.0f, 0.0f));
        resolveStageOutput("shininess", fragmentOutputs->shininessRegister, MaterialDataType::FLOAT1, glm::vec4(32.0f, 0.0f, 0.0f, 0.0f));
    }

    outProgram.instructions = std::move(compiledInstructions);
    return true;
}

std::string EmitStageInstructions(const MaterialStageProgram& program, bool vertexStage) {
    std::ostringstream code;
    for (int registerIndex = 0; registerIndex < program.registerCount; registerIndex++) {
        code << "    vec4 r" << registerIndex << " = vec4(0.0);\n";
    }

    for (const MaterialInstruction& instruction : program.instructions) {
        code << "    r" << instruction.dstRegister << " = ";
        switch (instruction.opcode) {
        case MaterialOpcode::CONSTANT:
            code << WrapGlslRegisterValue(GlslValueLiteral(instruction.immediate, instruction.resultType), instruction.resultType);
            break;
        case MaterialOpcode::PARAMETER: {
            const std::string paramRef = "u_MaterialParams[" + std::to_string(instruction.parameterIndex) + "]";
            const std::string expression =
                instruction.resultType == MaterialDataType::FLOAT1 || instruction.resultType == MaterialDataType::BOOL1
                    ? paramRef + ".x"
                    : instruction.resultType == MaterialDataType::VEC2 ? paramRef + ".xy"
                    : instruction.resultType == MaterialDataType::VEC3 ? paramRef + ".xyz"
                                                                      : paramRef;
            code << WrapGlslRegisterValue(expression, instruction.resultType);
            break;
        }
        case MaterialOpcode::SEMANTIC: {
            const std::string expression =
                vertexStage ? VertexSemanticExpression(instruction.semantic) : FragmentSemanticExpression(instruction.semantic);
            code << WrapGlslRegisterValue(expression, instruction.resultType);
            break;
        }
        case MaterialOpcode::ADD:
            code << WrapGlslRegisterValue("(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + " + " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::SUBTRACT:
            code << WrapGlslRegisterValue("(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + " - " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::MULTIPLY:
            code << WrapGlslRegisterValue("(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + " * " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::DIVIDE:
            code << WrapGlslRegisterValue("(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + " / max(" +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ", " +
                                              GlslValueLiteral(glm::vec4(1e-6f, 1e-6f, 1e-6f, 1e-6f), instruction.resultType) + "))",
                                          instruction.resultType);
            break;
        case MaterialOpcode::MINIMUM:
            code << WrapGlslRegisterValue("min(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ", " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::MAXIMUM:
            code << WrapGlslRegisterValue("max(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ", " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::CLAMP:
            code << WrapGlslRegisterValue("clamp(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ", " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ", " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[2]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::SATURATE:
            code << WrapGlslRegisterValue("clamp(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ", 0.0, 1.0)",
                                          instruction.resultType);
            break;
        case MaterialOpcode::ABSOLUTE:
            code << WrapGlslRegisterValue("abs(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ")", instruction.resultType);
            break;
        case MaterialOpcode::FLOOR:
            code << WrapGlslRegisterValue("floor(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ")", instruction.resultType);
            break;
        case MaterialOpcode::FRACTION:
            code << WrapGlslRegisterValue("fract(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ")", instruction.resultType);
            break;
        case MaterialOpcode::SINE:
            code << WrapGlslRegisterValue("sin(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ")", instruction.resultType);
            break;
        case MaterialOpcode::COSINE:
            code << WrapGlslRegisterValue("cos(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ")", instruction.resultType);
            break;
        case MaterialOpcode::DOT:
            code << WrapGlslRegisterValue("dot(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ", " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::NORMALIZE:
            code << WrapGlslRegisterValue("normalize(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::LENGTH:
            code << WrapGlslRegisterValue("length(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ")", instruction.resultType);
            break;
        case MaterialOpcode::POWER:
            code << WrapGlslRegisterValue("pow(max(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ", 0.0), max(" +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ", 0.0))",
                                          instruction.resultType);
            break;
        case MaterialOpcode::LERP:
            code << WrapGlslRegisterValue("mix(" + ReadRegisterExpression(program, instruction.srcRegisters[0]) + ", " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[1]) + ", " +
                                              ReadRegisterExpression(program, instruction.srcRegisters[2]) + ")",
                                          instruction.resultType);
            break;
        case MaterialOpcode::APPEND: {
            code << "vec4(";
            int writtenComponents = 0;
            for (size_t inputIndex = 0; inputIndex < instruction.srcRegisters.size() && writtenComponents < instruction.componentCount; inputIndex++) {
                if (instruction.srcRegisters[inputIndex] < 0) {
                    continue;
                }
                if (writtenComponents > 0) {
                    code << ", ";
                }
                code << "r" << instruction.srcRegisters[inputIndex] << ".x";
                writtenComponents++;
            }
            while (writtenComponents < 4) {
                if (writtenComponents > 0) {
                    code << ", ";
                }
                code << "0.0";
                writtenComponents++;
            }
            code << ")";
            break;
        }
        case MaterialOpcode::SWIZZLE:
            code << WrapGlslRegisterValue("r" + std::to_string(instruction.srcRegisters[0]) + SwizzleSuffix(instruction), instruction.resultType);
            break;
        case MaterialOpcode::SAMPLE_TEXTURE:
            code << "texture(u_MaterialSampler" << instruction.samplerIndex << ", r" << instruction.srcRegisters[0] << ".xy)";
            break;
        }
        code << ";\n";
    }
    return code.str();
}

std::shared_ptr<const RenderShaderSnapshot> GenerateGlShader(const CompiledMaterialTemplate& material) {
    std::ostringstream vertex;
    vertex << "layout(location = 0) in vec4 a_Position;\n";
    vertex << "layout(location = 1) in vec3 a_Normal;\n";
    vertex << "layout(location = 2) in vec2 a_TexCoord;\n";
    vertex << "layout(location = 3) in vec4 a_Color;\n";
    vertex << "uniform mat4 u_Model;\n";
    vertex << "uniform mat4 u_View;\n";
    vertex << "uniform mat4 u_Projection;\n";
    vertex << "uniform float u_Time;\n";
    if (!material.parameters.empty()) {
        vertex << "uniform vec4 u_MaterialParams[" << material.parameters.size() << "];\n";
    }
    vertex << "out vec3 v_WorldPos;\n";
    vertex << "out vec3 v_NormalWS;\n";
    vertex << "out vec2 v_TexCoord;\n";
    vertex << "out vec4 v_Color;\n";
    for (int varyingIndex = 0; varyingIndex < material.varyingCount; varyingIndex++) {
        vertex << "out vec4 v_Varying" << varyingIndex << ";\n";
    }
    vertex << "void main() {\n";
    vertex << EmitStageInstructions(material.vertexProgram, true);
    vertex << "    vec4 rrPositionOS = " << ResolveVertexOutputExpression(material.vertexProgram,
                                                                           material.vertexOutputs.positionRegister,
                                                                           "a_Position")
           << ";\n";
    vertex << "    vec3 rrNormalOS = " << ResolveVertexOutputExpression(material.vertexProgram,
                                                                         material.vertexOutputs.normalRegister,
                                                                         "a_Normal")
           << ";\n";
    vertex << "    vec2 rrUv0 = " << ResolveVertexOutputExpression(material.vertexProgram,
                                                                    material.vertexOutputs.uvRegister,
                                                                    "a_TexCoord")
           << ";\n";
    vertex << "    vec4 rrColor0 = " << ResolveVertexOutputExpression(material.vertexProgram,
                                                                       material.vertexOutputs.colorRegister,
                                                                       "a_Color")
           << ";\n";
    vertex << "    vec4 rrWorldPosition = u_Model * rrPositionOS;\n";
    vertex << "    mat3 rrNormalMatrix = transpose(inverse(mat3(u_Model)));\n";
    vertex << "    v_WorldPos = rrWorldPosition.xyz;\n";
    vertex << "    v_NormalWS = normalize(rrNormalMatrix * rrNormalOS);\n";
    vertex << "    v_TexCoord = rrUv0;\n";
    vertex << "    v_Color = rrColor0;\n";
    for (int varyingIndex = 0; varyingIndex < material.varyingCount; varyingIndex++) {
        const int registerIndex = material.vertexOutputs.varyingRegisters[static_cast<size_t>(varyingIndex)];
        vertex << "    v_Varying" << varyingIndex << " = "
               << (registerIndex >= 0 ? "r" + std::to_string(registerIndex) : "vec4(0.0)") << ";\n";
    }
    vertex << "    gl_Position = u_Projection * u_View * rrWorldPosition;\n";
    vertex << "}\n";

    std::ostringstream fragment;
    fragment << "in vec3 v_WorldPos;\n";
    fragment << "in vec3 v_NormalWS;\n";
    fragment << "in vec2 v_TexCoord;\n";
    fragment << "in vec4 v_Color;\n";
    for (int varyingIndex = 0; varyingIndex < material.varyingCount; varyingIndex++) {
        fragment << "in vec4 v_Varying" << varyingIndex << ";\n";
    }
    fragment << "uniform vec3 u_LightPos;\n";
    fragment << "uniform vec3 u_ViewPos;\n";
    fragment << "uniform vec3 u_LightColor;\n";
    fragment << "uniform float u_LightIntensity;\n";
    fragment << "uniform vec2 u_FrameSize;\n";
    fragment << "uniform float u_AlphaCutoff;\n";
    if (!material.parameters.empty()) {
        fragment << "uniform vec4 u_MaterialParams[" << material.parameters.size() << "];\n";
    }
    for (size_t samplerIndex = 0; samplerIndex < material.samplers.size(); samplerIndex++) {
        fragment << "uniform sampler2D u_MaterialSampler" << samplerIndex << ";\n";
    }
    fragment << "out vec4 FragColor;\n";
    fragment << "float rrComputeAttenuation(float lightDistance) {\n";
    fragment << "    return 1.0 / (1.0 + 0.09 * lightDistance + 0.032 * lightDistance * lightDistance);\n";
    fragment << "}\n";
    fragment << "void main() {\n";
    fragment << EmitStageInstructions(material.fragmentProgram, false);
    fragment << "    vec4 rrBaseColor = "
             << ResolveFragmentOutputExpression(
                    material.fragmentProgram, material.fragmentOutputs.baseColorRegister, MaterialDataType::VEC4, "vec4(1.0)")
             << ";\n";
    fragment << "    vec3 rrEmissive = "
             << ResolveFragmentOutputExpression(
                    material.fragmentProgram, material.fragmentOutputs.emissiveRegister, MaterialDataType::VEC3, "vec3(0.0)")
             << ";\n";
    fragment << "    float rrAlpha = "
             << ResolveFragmentOutputExpression(
                    material.fragmentProgram, material.fragmentOutputs.alphaRegister, MaterialDataType::FLOAT1, "rrBaseColor.a")
             << ";\n";
    fragment << "    float rrAmbientStrength = "
             << ResolveFragmentOutputExpression(material.fragmentProgram,
                                                material.fragmentOutputs.ambientStrengthRegister,
                                                MaterialDataType::FLOAT1,
                                                "0.3")
             << ";\n";
    fragment << "    float rrSpecularStrength = "
             << ResolveFragmentOutputExpression(material.fragmentProgram,
                                                material.fragmentOutputs.specularStrengthRegister,
                                                MaterialDataType::FLOAT1,
                                                "0.3")
             << ";\n";
    fragment << "    float rrShininess = "
             << ResolveFragmentOutputExpression(
                    material.fragmentProgram, material.fragmentOutputs.shininessRegister, MaterialDataType::FLOAT1, "32.0")
             << ";\n";
    if (material.pipelineState.blendMode == MaterialBlendMode::ALPHA_CUTOUT) {
        fragment << "    if (rrAlpha < u_AlphaCutoff) { discard; }\n";
    }
    fragment << "    vec3 rrLighting = vec3(1.0);\n";
    if (material.pipelineState.shadingModel != MaterialShadingModel::UNLIT) {
        fragment << "    vec3 rrNormal = normalize(v_NormalWS);\n";
        fragment << "    vec3 rrLightVector = u_LightPos - v_WorldPos;\n";
        fragment << "    float rrLightDistance = length(rrLightVector);\n";
        fragment << "    vec3 rrLightDir = rrLightDistance > 0.00001 ? rrLightVector / rrLightDistance : vec3(0.0, 0.0, 1.0);\n";
        fragment << "    float rrAttenuation = rrComputeAttenuation(rrLightDistance) * max(u_LightIntensity, 0.0);\n";
        fragment << "    float rrDiff = max(dot(rrNormal, rrLightDir), 0.0);\n";
        fragment << "    rrLighting = rrAmbientStrength * u_LightColor + rrDiff * u_LightColor * rrAttenuation;\n";
        if (material.pipelineState.shadingModel == MaterialShadingModel::PHONG) {
            fragment << "    vec3 rrViewDir = normalize(u_ViewPos - v_WorldPos);\n";
            fragment << "    vec3 rrReflectDir = reflect(-rrLightDir, rrNormal);\n";
            fragment << "    float rrSpec = pow(max(dot(rrViewDir, rrReflectDir), 0.0), max(rrShininess, 1.0));\n";
            fragment << "    rrLighting += rrSpecularStrength * rrSpec * u_LightColor * rrAttenuation;\n";
        }
    }
    fragment << "    vec3 rrColor = rrBaseColor.rgb * rrLighting + rrEmissive;\n";
    fragment << "    FragColor = vec4(rrColor, clamp(rrAlpha, 0.0, 1.0));\n";
    fragment << "}\n";

    auto shader = std::make_shared<RenderShaderSnapshot>();
    shader->vertexCode = vertex.str();
    shader->fragmentCode = fragment.str();
    return shader;
}

std::shared_ptr<const CompiledMaterialTemplate> MakeErrorTemplate(const std::filesystem::path& path) {
    auto compiled = std::make_shared<CompiledMaterialTemplate>();
    compiled->name = "Error material";
    compiled->assetPath = path;
    compiled->pipelineState.shadingModel = MaterialShadingModel::UNLIT;

    MaterialInstruction position{};
    position.opcode = MaterialOpcode::SEMANTIC;
    position.resultType = MaterialDataType::VEC4;
    position.semantic = MaterialSemantic::POSITION_OS;
    position.componentCount = 4;
    compiled->vertexOutputs.positionRegister = AppendInstruction(compiled->vertexProgram, compiled->vertexProgram.instructions, position);

    MaterialInstruction normal{};
    normal.opcode = MaterialOpcode::SEMANTIC;
    normal.resultType = MaterialDataType::VEC3;
    normal.semantic = MaterialSemantic::NORMAL_OS;
    normal.componentCount = 3;
    compiled->vertexOutputs.normalRegister = AppendInstruction(compiled->vertexProgram, compiled->vertexProgram.instructions, normal);

    MaterialInstruction uv{};
    uv.opcode = MaterialOpcode::SEMANTIC;
    uv.resultType = MaterialDataType::VEC2;
    uv.semantic = MaterialSemantic::UV0;
    uv.componentCount = 2;
    compiled->vertexOutputs.uvRegister = AppendInstruction(compiled->vertexProgram, compiled->vertexProgram.instructions, uv);

    MaterialInstruction color{};
    color.opcode = MaterialOpcode::SEMANTIC;
    color.resultType = MaterialDataType::VEC4;
    color.semantic = MaterialSemantic::COLOR0;
    color.componentCount = 4;
    compiled->vertexOutputs.colorRegister = AppendInstruction(compiled->vertexProgram, compiled->vertexProgram.instructions, color);

    compiled->fragmentOutputs.baseColorRegister =
        AppendConstant(compiled->fragmentProgram, compiled->fragmentProgram.instructions, MaterialDataType::VEC4, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    compiled->fragmentOutputs.emissiveRegister =
        AppendConstant(compiled->fragmentProgram, compiled->fragmentProgram.instructions, MaterialDataType::VEC3, glm::vec4(0.0f));
    compiled->fragmentOutputs.alphaRegister =
        AppendConstant(compiled->fragmentProgram, compiled->fragmentProgram.instructions, MaterialDataType::FLOAT1, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    compiled->fragmentOutputs.ambientStrengthRegister =
        AppendConstant(compiled->fragmentProgram, compiled->fragmentProgram.instructions, MaterialDataType::FLOAT1, glm::vec4(0.0f));
    compiled->fragmentOutputs.specularStrengthRegister =
        AppendConstant(compiled->fragmentProgram, compiled->fragmentProgram.instructions, MaterialDataType::FLOAT1, glm::vec4(0.0f));
    compiled->fragmentOutputs.shininessRegister =
        AppendConstant(compiled->fragmentProgram, compiled->fragmentProgram.instructions, MaterialDataType::FLOAT1, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    compiled->glShader = GenerateGlShader(*compiled);
    return compiled;
}
} // namespace

void MaterialManager::BindRenderServices(IRenderInvalidationSink& renderInvalidationSink) {
    p_RenderInvalidationSink_ = &renderInvalidationSink;
}

void MaterialManager::BindSceneAccessor(std::function<std::shared_ptr<Scene>()> sceneAccessor) {
    m_SceneAccessor = std::move(sceneAccessor);
}

bool MaterialManager::Init() {
    if (p_RenderInvalidationSink_ == nullptr) {
        LOGE("MaterialManager render services were not bound before initialization");
        return false;
    }
    return true;
}

void MaterialManager::LoadTexture(const std::string& path) {
    Texture texture;
    if (!texture.LoadFromFile(path.c_str())) {
        LOGE("Failed to load texture %s", path.c_str());
        return;
    }
    LoadTextureIntoSelectedMaterial(std::move(texture));
}

void MaterialManager::LoadTexture(const uint8_t* data, const size_t size) {
    Texture texture;
    if (!texture.LoadFromMemory(data, size)) {
        LOGE("Failed to load texture from memory");
        return;
    }
    LoadTextureIntoSelectedMaterial(std::move(texture));
}

void MaterialManager::ClearTexture() {
    SceneMaterial* material = ResolveSelectedSceneMaterial();
    if (material == nullptr) {
        return;
    }
    if (!material->textureBindings.empty()) {
        material->textureBindings.front().texture = Texture{};
    }
    if (p_RenderInvalidationSink_ != nullptr) {
        p_RenderInvalidationSink_->OnTextureMutated();
        p_RenderInvalidationSink_->OnSceneMutated();
    }
}

void MaterialManager::SelectSceneMaterial(SceneMaterialHandle handle) {
    m_SelectedSceneMaterialHandle = handle;
}

SceneMaterial* MaterialManager::GetSelectedSceneMaterial() {
    return ResolveSelectedSceneMaterial();
}

const SceneMaterial* MaterialManager::GetSelectedSceneMaterial() const {
    return ResolveSelectedSceneMaterial();
}

const Texture* MaterialManager::GetSelectedPreviewTexture() const {
    const SceneMaterial* material = ResolveSelectedSceneMaterial();
    if (material == nullptr || material->textureBindings.empty()) {
        return nullptr;
    }
    return &material->textureBindings.front().texture;
}

std::shared_ptr<const CompiledMaterialTemplate> MaterialManager::GetCompiledTemplate(const SceneMaterial& material) const {
    return GetCompiledTemplate(material.templatePath);
}

std::shared_ptr<const CompiledMaterialTemplate> MaterialManager::GetCompiledTemplate(const std::filesystem::path& templatePath) const {
    const std::filesystem::path resolvedPath = ResolveTemplateAssetPath(templatePath);
    const std::string cacheKey = resolvedPath.generic_string();
    auto it = m_TemplateCache.find(cacheKey);
    if (it != m_TemplateCache.end()) {
        return it->second;
    }

    std::shared_ptr<const CompiledMaterialTemplate> compiled = LoadAndCompileTemplate(resolvedPath);
    m_TemplateCache.emplace(cacheKey, compiled);
    return compiled;
}

std::filesystem::path MaterialManager::ResolveBuiltInTemplatePath(bool useVertexColor) const {
    return useVertexColor ? std::filesystem::path(kMaterialAssetPhongVertexColor) : std::filesystem::path(kMaterialAssetPhongTextured);
}

ShaderProgram MaterialManager::CreateShaderProgram(IShaderCompiler& shaderCompiler,
                                                   const std::string& vertexPath,
                                                   const std::string& fragmentPath) {
    std::string vertexCode = ReadTextFile(AssetsRootPath() / vertexPath);
    std::string fragmentCode = ReadTextFile(AssetsRootPath() / fragmentPath);
    if (vertexCode.empty() || fragmentCode.empty()) {
        return {ShaderHandle{}, nullptr, "Invalid Shader", vertexPath, fragmentPath};
    }

    ShaderHandle handle = shaderCompiler.CompileShaders(vertexCode, fragmentCode);
    if (!handle.IsValid()) {
        return {ShaderHandle{}, nullptr, "Invalid Shader", vertexPath, fragmentPath};
    }

    return {
        .handle = handle,
        .source = nullptr,
        .name = vertexPath + "+" + fragmentPath,
        .vertexPath = vertexPath,
        .fragmentPath = fragmentPath,
    };
}

ShaderProgram MaterialManager::CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = ReadTextFile(AssetsRootPath() / vertexPath);
    std::string fragmentCode = ReadTextFile(AssetsRootPath() / fragmentPath);
    if (vertexCode.empty() || fragmentCode.empty()) {
        return {ShaderHandle{}, nullptr, "Invalid Shader", vertexPath, fragmentPath};
    }

    auto source = std::make_shared<RenderShaderSnapshot>();
    source->vertexCode = std::move(vertexCode);
    source->fragmentCode = std::move(fragmentCode);
    return {ShaderHandle{}, std::move(source), vertexPath + "+" + fragmentPath, vertexPath, fragmentPath};
}

std::string MaterialManager::ReadTextFile(const std::string& path) {
    return ReadTextFile(std::filesystem::path(path));
}

std::string MaterialManager::ReadTextFile(const std::filesystem::path& path) {
#ifdef __ANDROID__
    AAsset* asset = AAssetManager_open(g_assetManager, path.generic_string().c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open text asset %s", path.string().c_str());
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
        LOGE("Failed to open text file %s", path.string().c_str());
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
#endif
}

std::shared_ptr<const CompiledMaterialTemplate> MaterialManager::LoadAndCompileTemplate(const std::filesystem::path& templatePath) const {
    const std::string sourceText = ReadTextFile(templatePath);
    MaterialTemplateSource source{};
    if (!ParseTemplateSource(templatePath, sourceText, source)) {
        return MakeErrorTemplate(templatePath);
    }

    auto compiled = std::make_shared<CompiledMaterialTemplate>();
    compiled->name = source.name;
    compiled->assetPath = templatePath;
    compiled->pipelineState = source.pipelineState;
    compiled->parameters = source.parameters;
    compiled->samplers = source.samplers;
    compiled->cacheKey = HashSourceText(sourceText);

    if (!CompileStageProgram(source,
                             source.vertex,
                             true,
                             compiled->vertexProgram,
                             &compiled->vertexOutputs,
                             nullptr,
                             compiled->usesVertexColor,
                             compiled->usesTextureSampling,
                             compiled->varyingCount) ||
        !CompileStageProgram(source,
                             source.fragment,
                             false,
                             compiled->fragmentProgram,
                             nullptr,
                             &compiled->fragmentOutputs,
                             compiled->usesVertexColor,
                             compiled->usesTextureSampling,
                             compiled->varyingCount)) {
        return MakeErrorTemplate(templatePath);
    }

    compiled->glShader = GenerateGlShader(*compiled);
    return compiled;
}

void MaterialManager::LoadTextureIntoSelectedMaterial(Texture texture) {
    SceneMaterial* material = ResolveSelectedSceneMaterial();
    if (material == nullptr) {
        LOGW("No selected scene material to receive the loaded texture");
        return;
    }

    if (material->textureBindings.empty()) {
        material->textureBindings.push_back(MaterialTextureBinding{
            .slotName = "albedo",
            .texture = std::move(texture),
        });
    } else {
        material->textureBindings.front().texture = std::move(texture);
    }

    if (p_RenderInvalidationSink_ != nullptr) {
        p_RenderInvalidationSink_->OnTextureMutated();
        p_RenderInvalidationSink_->OnSceneMutated();
    }
}

SceneMaterial* MaterialManager::ResolveSelectedSceneMaterial() {
    if (!m_SceneAccessor) {
        return nullptr;
    }
    const std::shared_ptr<Scene> scene = m_SceneAccessor();
    if (!scene) {
        return nullptr;
    }
    if (m_SelectedSceneMaterialHandle == kInvalidSceneMaterialHandle && scene->GetMaterialCount() > 0) {
        m_SelectedSceneMaterialHandle = 0;
    }
    return scene->GetMaterial(m_SelectedSceneMaterialHandle);
}

const SceneMaterial* MaterialManager::ResolveSelectedSceneMaterial() const {
    if (!m_SceneAccessor) {
        return nullptr;
    }
    const std::shared_ptr<Scene> scene = m_SceneAccessor();
    if (!scene) {
        return nullptr;
    }
    const SceneMaterialHandle handle = m_SelectedSceneMaterialHandle == kInvalidSceneMaterialHandle && scene->GetMaterialCount() > 0
                                           ? 0
                                           : m_SelectedSceneMaterialHandle;
    return scene->GetMaterial(handle);
}

std::filesystem::path MaterialManager::ResolveTemplateAssetPath(const std::filesystem::path& templatePath) const {
    if (templatePath.is_absolute()) {
        return templatePath;
    }
    const auto begin = templatePath.begin();
    if (begin != templatePath.end() && *begin == AssetsRootPath().filename()) {
        return templatePath;
    }
    return AssetsRootPath() / templatePath;
}

} // namespace RetroRenderer
