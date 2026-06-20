#pragma once

#include "../Scene/Texture.h"
#include "ShaderResource.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec4.hpp>

namespace RetroRenderer {

enum class MaterialDataType {
    FLOAT1,
    VEC2,
    VEC3,
    VEC4,
    BOOL1,
};

enum class MaterialShadingModel {
    UNLIT,
    LAMBERT,
    PHONG,
};

enum class MaterialBlendMode {
    OPAQUE,
    ALPHA_BLEND,
    ALPHA_CUTOUT,
};

enum class MaterialCullMode {
    NONE,
    BACK,
    FRONT,
};

enum class MaterialWrapMode {
    REPEAT,
    CLAMP_TO_EDGE,
};

enum class MaterialFilterMode {
    NEAREST,
    LINEAR,
};

enum class MaterialSemantic {
    POSITION_OS,
    NORMAL_OS,
    UV0,
    COLOR0,
    TIME,
    WORLD_POSITION,
    NORMAL_WS,
    VIEW_DIR_WS,
    SCREEN_UV,
    VARYING0,
    VARYING1,
    VARYING2,
    VARYING3,
};

enum class MaterialOpcode {
    CONSTANT,
    PARAMETER,
    SEMANTIC,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    MINIMUM,
    MAXIMUM,
    CLAMP,
    SATURATE,
    ABSOLUTE,
    FLOOR,
    FRACTION,
    SINE,
    COSINE,
    DOT,
    NORMALIZE,
    LENGTH,
    POWER,
    LERP,
    APPEND,
    SWIZZLE,
    SAMPLE_TEXTURE,
};

struct MaterialValue {
    MaterialDataType type = MaterialDataType::VEC4;
    glm::vec4 data = glm::vec4(0.0f);
};

struct MaterialParameterDesc {
    std::string name;
    MaterialDataType type = MaterialDataType::VEC4;
    MaterialValue defaultValue{};
};

struct MaterialSamplerDesc {
    std::string name;
    MaterialFilterMode filter = MaterialFilterMode::LINEAR;
    MaterialWrapMode wrapU = MaterialWrapMode::REPEAT;
    MaterialWrapMode wrapV = MaterialWrapMode::REPEAT;
};

struct MaterialPipelineState {
    MaterialShadingModel shadingModel = MaterialShadingModel::PHONG;
    MaterialBlendMode blendMode = MaterialBlendMode::OPAQUE;
    MaterialCullMode cullMode = MaterialCullMode::BACK;
    bool depthTest = true;
    bool depthWrite = true;
    float alphaCutoff = 0.5f;
    float boundsPadding = 0.0f;
};

struct MaterialInstruction {
    MaterialOpcode opcode = MaterialOpcode::CONSTANT;
    MaterialDataType resultType = MaterialDataType::VEC4;
    int dstRegister = -1;
    std::array<int, 4> srcRegisters = {-1, -1, -1, -1};
    glm::vec4 immediate = glm::vec4(0.0f);
    int parameterIndex = -1;
    int samplerIndex = -1;
    MaterialSemantic semantic = MaterialSemantic::POSITION_OS;
    std::array<uint8_t, 4> swizzle = {0, 1, 2, 3};
    uint8_t componentCount = 4;
};

struct MaterialVertexOutputs {
    int positionRegister = -1;
    int normalRegister = -1;
    int uvRegister = -1;
    int colorRegister = -1;
    std::array<int, 4> varyingRegisters = {-1, -1, -1, -1};
};

struct MaterialFragmentOutputs {
    int baseColorRegister = -1;
    int emissiveRegister = -1;
    int alphaRegister = -1;
    int ambientStrengthRegister = -1;
    int specularStrengthRegister = -1;
    int shininessRegister = -1;
};

struct MaterialStageProgram {
    std::vector<MaterialInstruction> instructions;
    int registerCount = 0;
    std::vector<MaterialDataType> registerTypes;
};

struct CompiledMaterialTemplate {
    std::string name;
    std::filesystem::path assetPath;
    MaterialPipelineState pipelineState{};
    std::vector<MaterialParameterDesc> parameters;
    std::vector<MaterialSamplerDesc> samplers;
    MaterialStageProgram vertexProgram;
    MaterialStageProgram fragmentProgram;
    MaterialVertexOutputs vertexOutputs{};
    MaterialFragmentOutputs fragmentOutputs{};
    std::shared_ptr<const RenderShaderSnapshot> glShader;
    uint64_t cacheKey = 0;
    bool usesVertexColor = false;
    bool usesTextureSampling = false;
    int varyingCount = 0;
};

struct MaterialParameterOverride {
    std::string name;
    MaterialValue value{};
};

struct MaterialTextureBinding {
    std::string slotName;
    std::shared_ptr<const Texture> texture;
};

struct SceneMaterialOverrides {
    std::optional<MaterialBlendMode> blendMode;
    std::optional<MaterialCullMode> cullMode;
    std::optional<bool> depthTest;
    std::optional<bool> depthWrite;
    std::optional<float> alphaCutoff;
};

using SceneMaterialHandle = uint32_t;
constexpr SceneMaterialHandle kInvalidSceneMaterialHandle = std::numeric_limits<SceneMaterialHandle>::max();

struct SceneMaterial {
    std::string name;
    std::filesystem::path templatePath;
    std::vector<MaterialParameterOverride> parameterOverrides;
    std::vector<MaterialTextureBinding> textureBindings;
    SceneMaterialOverrides pipelineOverrides{};
};

inline constexpr const char* kMaterialAssetPhongTextured = "materials/phong-textured.rrmatdef.json";
inline constexpr const char* kMaterialAssetPhongVertexColor = "materials/phong-vertex-color.rrmatdef.json";
inline constexpr const char* kMaterialAssetPhongConstant = "materials/phong-constant.rrmatdef.json";
inline constexpr const char* kMaterialAssetUnlitTextured = "materials/unlit-textured.rrmatdef.json";
inline constexpr const char* kMaterialAssetUnlitVertexColor = "materials/unlit-vertex-color.rrmatdef.json";

} // namespace RetroRenderer
