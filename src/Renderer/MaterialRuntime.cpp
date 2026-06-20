#include "MaterialRuntime.h"

#include <algorithm>
#include <cmath>

namespace RetroRenderer {
namespace {

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

glm::vec4 TruncateValue(glm::vec4 value, MaterialDataType type) {
    switch (type) {
    case MaterialDataType::FLOAT1:
    case MaterialDataType::BOOL1:
        value.y = 0.0f;
        value.z = 0.0f;
        value.w = 0.0f;
        break;
    case MaterialDataType::VEC2:
        value.z = 0.0f;
        value.w = 0.0f;
        break;
    case MaterialDataType::VEC3:
        value.w = 0.0f;
        break;
    case MaterialDataType::VEC4:
        break;
    }
    return value;
}

glm::vec4 MaterialSemanticValue(MaterialSemantic semantic, const MaterialVertexStageInput& input) {
    switch (semantic) {
    case MaterialSemantic::POSITION_OS:
        return input.positionOS;
    case MaterialSemantic::NORMAL_OS:
        return glm::vec4(input.normalOS, 0.0f);
    case MaterialSemantic::UV0:
        return glm::vec4(input.uv0, 0.0f, 0.0f);
    case MaterialSemantic::COLOR0:
        return input.color0;
    case MaterialSemantic::TIME:
        return glm::vec4(input.time, 0.0f, 0.0f, 0.0f);
    default:
        break;
    }
    return glm::vec4(0.0f);
}

glm::vec4 MaterialSemanticValue(MaterialSemantic semantic, const MaterialFragmentStageInput& input) {
    switch (semantic) {
    case MaterialSemantic::WORLD_POSITION:
        return glm::vec4(input.worldPosition, 0.0f);
    case MaterialSemantic::NORMAL_WS:
        return glm::vec4(input.normalWS, 0.0f);
    case MaterialSemantic::UV0:
        return glm::vec4(input.uv0, 0.0f, 0.0f);
    case MaterialSemantic::COLOR0:
        return input.color0;
    case MaterialSemantic::VIEW_DIR_WS:
        return glm::vec4(input.viewDirWS, 0.0f);
    case MaterialSemantic::SCREEN_UV:
        return glm::vec4(input.screenUV, 0.0f, 0.0f);
    case MaterialSemantic::VARYING0:
        return input.varyings[0];
    case MaterialSemantic::VARYING1:
        return input.varyings[1];
    case MaterialSemantic::VARYING2:
        return input.varyings[2];
    case MaterialSemantic::VARYING3:
        return input.varyings[3];
    default:
        break;
    }
    return glm::vec4(0.0f);
}

float WrapCoordinate(float value, MaterialWrapMode wrapMode) {
    if (wrapMode == MaterialWrapMode::CLAMP_TO_EDGE) {
        return std::clamp(value, 0.0f, 1.0f);
    }
    return value - std::floor(value);
}

Pixel SampleNearest(const ResolvedMaterialSampler& sampler, const glm::vec2& uv) {
    if (sampler.texture == nullptr || !sampler.texture->HasCpuPixels()) {
        return Pixel{255, 255, 255, 255};
    }

    const float wrappedU = WrapCoordinate(uv.x, sampler.wrapU);
    const float wrappedV = WrapCoordinate(uv.y, sampler.wrapV);
    const int width = sampler.texture->GetWidth();
    const int height = sampler.texture->GetHeight();
    const int x = std::clamp(static_cast<int>(std::floor(wrappedU * static_cast<float>(width))), 0, width - 1);
    const int y = std::clamp(static_cast<int>(std::floor((1.0f - wrappedV) * static_cast<float>(height))), 0, height - 1);
    return sampler.texture->GetPixels()[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
}

glm::vec4 PixelToUnitVec4(const Pixel& pixel) {
    return glm::vec4(pixel.r / 255.0f, pixel.g / 255.0f, pixel.b / 255.0f, pixel.a / 255.0f);
}

Pixel SampleLinear(const ResolvedMaterialSampler& sampler, const glm::vec2& uv) {
    if (sampler.texture == nullptr || !sampler.texture->HasCpuPixels()) {
        return Pixel{255, 255, 255, 255};
    }

    const float wrappedU = WrapCoordinate(uv.x, sampler.wrapU);
    const float wrappedV = WrapCoordinate(uv.y, sampler.wrapV);
    const int width = sampler.texture->GetWidth();
    const int height = sampler.texture->GetHeight();
    const float sampleX = wrappedU * static_cast<float>(width - 1);
    const float sampleY = (1.0f - wrappedV) * static_cast<float>(height - 1);

    const int x0 = std::clamp(static_cast<int>(std::floor(sampleX)), 0, width - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(sampleY)), 0, height - 1);
    const int x1 = std::clamp(x0 + 1, 0, width - 1);
    const int y1 = std::clamp(y0 + 1, 0, height - 1);
    const float tx = sampleX - static_cast<float>(x0);
    const float ty = sampleY - static_cast<float>(y0);

    const auto& pixels = sampler.texture->GetPixels();
    const glm::vec4 p00 = PixelToUnitVec4(pixels[static_cast<size_t>(y0) * static_cast<size_t>(width) + static_cast<size_t>(x0)]);
    const glm::vec4 p10 = PixelToUnitVec4(pixels[static_cast<size_t>(y0) * static_cast<size_t>(width) + static_cast<size_t>(x1)]);
    const glm::vec4 p01 = PixelToUnitVec4(pixels[static_cast<size_t>(y1) * static_cast<size_t>(width) + static_cast<size_t>(x0)]);
    const glm::vec4 p11 = PixelToUnitVec4(pixels[static_cast<size_t>(y1) * static_cast<size_t>(width) + static_cast<size_t>(x1)]);

    const glm::vec4 top = glm::mix(p00, p10, tx);
    const glm::vec4 bottom = glm::mix(p01, p11, tx);
    const glm::vec4 sampled = glm::mix(top, bottom, ty);
    return Pixel{
        static_cast<uint8_t>(std::clamp(std::lround(sampled.r * 255.0f), 0L, 255L)),
        static_cast<uint8_t>(std::clamp(std::lround(sampled.g * 255.0f), 0L, 255L)),
        static_cast<uint8_t>(std::clamp(std::lround(sampled.b * 255.0f), 0L, 255L)),
        static_cast<uint8_t>(std::clamp(std::lround(sampled.a * 255.0f), 0L, 255L)),
    };
}

template <typename TInput>
void ExecuteProgram(const MaterialStageProgram& program,
                    const std::vector<glm::vec4>& parameterValues,
                    const std::vector<ResolvedMaterialSampler>* samplers,
                    const TInput& input,
                    std::vector<glm::vec4>& registers) {
    registers.assign(static_cast<size_t>(program.registerCount), glm::vec4(0.0f));

    for (const MaterialInstruction& instruction : program.instructions) {
        glm::vec4 value(0.0f);
        const auto read = [&](int registerIndex) -> glm::vec4 {
            return registerIndex >= 0 && registerIndex < static_cast<int>(registers.size()) ? registers[static_cast<size_t>(registerIndex)]
                                                                                             : glm::vec4(0.0f);
        };

        switch (instruction.opcode) {
        case MaterialOpcode::CONSTANT:
            value = instruction.immediate;
            break;
        case MaterialOpcode::PARAMETER:
            value = instruction.parameterIndex >= 0 && instruction.parameterIndex < static_cast<int>(parameterValues.size())
                        ? parameterValues[static_cast<size_t>(instruction.parameterIndex)]
                        : glm::vec4(0.0f);
            break;
        case MaterialOpcode::SEMANTIC:
            if constexpr (std::is_same_v<TInput, MaterialVertexStageInput>) {
                value = MaterialSemanticValue(instruction.semantic, input);
            } else {
                value = MaterialSemanticValue(instruction.semantic, input);
            }
            break;
        case MaterialOpcode::ADD:
            value = read(instruction.srcRegisters[0]) + read(instruction.srcRegisters[1]);
            break;
        case MaterialOpcode::SUBTRACT:
            value = read(instruction.srcRegisters[0]) - read(instruction.srcRegisters[1]);
            break;
        case MaterialOpcode::MULTIPLY:
            value = read(instruction.srcRegisters[0]) * read(instruction.srcRegisters[1]);
            break;
        case MaterialOpcode::DIVIDE: {
            const glm::vec4 rhs = read(instruction.srcRegisters[1]);
            value = read(instruction.srcRegisters[0]) /
                    glm::vec4(std::max(std::abs(rhs.x), 1e-6f),
                              std::max(std::abs(rhs.y), 1e-6f),
                              std::max(std::abs(rhs.z), 1e-6f),
                              std::max(std::abs(rhs.w), 1e-6f));
            break;
        }
        case MaterialOpcode::MINIMUM:
            value = glm::min(read(instruction.srcRegisters[0]), read(instruction.srcRegisters[1]));
            break;
        case MaterialOpcode::MAXIMUM:
            value = glm::max(read(instruction.srcRegisters[0]), read(instruction.srcRegisters[1]));
            break;
        case MaterialOpcode::CLAMP:
            value = glm::clamp(read(instruction.srcRegisters[0]), read(instruction.srcRegisters[1]), read(instruction.srcRegisters[2]));
            break;
        case MaterialOpcode::SATURATE:
            value = glm::clamp(read(instruction.srcRegisters[0]), glm::vec4(0.0f), glm::vec4(1.0f));
            break;
        case MaterialOpcode::ABSOLUTE:
            value = glm::abs(read(instruction.srcRegisters[0]));
            break;
        case MaterialOpcode::FLOOR:
            value = glm::floor(read(instruction.srcRegisters[0]));
            break;
        case MaterialOpcode::FRACTION:
            value = glm::fract(read(instruction.srcRegisters[0]));
            break;
        case MaterialOpcode::SINE:
            value = glm::sin(read(instruction.srcRegisters[0]));
            break;
        case MaterialOpcode::COSINE:
            value = glm::cos(read(instruction.srcRegisters[0]));
            break;
        case MaterialOpcode::DOT: {
            const int lhsCount = ComponentCount(program.registerTypes[static_cast<size_t>(instruction.srcRegisters[0])]);
            const int rhsCount = ComponentCount(program.registerTypes[static_cast<size_t>(instruction.srcRegisters[1])]);
            const int count = std::min(lhsCount, rhsCount);
            const glm::vec4 lhs = read(instruction.srcRegisters[0]);
            const glm::vec4 rhs = read(instruction.srcRegisters[1]);
            float dotValue = lhs.x * rhs.x;
            if (count > 1) {
                dotValue += lhs.y * rhs.y;
            }
            if (count > 2) {
                dotValue += lhs.z * rhs.z;
            }
            if (count > 3) {
                dotValue += lhs.w * rhs.w;
            }
            value = glm::vec4(dotValue, 0.0f, 0.0f, 0.0f);
            break;
        }
        case MaterialOpcode::NORMALIZE: {
            glm::vec4 source = read(instruction.srcRegisters[0]);
            const int count = ComponentCount(program.registerTypes[static_cast<size_t>(instruction.srcRegisters[0])]);
            if (count <= 1) {
                const float absValue = std::abs(source.x);
                value = glm::vec4(absValue > 1e-6f ? source.x / absValue : 0.0f, 0.0f, 0.0f, 0.0f);
            } else if (count == 2) {
                const glm::vec2 normalized = glm::normalize(glm::vec2(source));
                value = glm::vec4(normalized, 0.0f, 0.0f);
            } else {
                const glm::vec3 normalized = glm::normalize(glm::vec3(source));
                value = glm::vec4(normalized, 0.0f);
            }
            break;
        }
        case MaterialOpcode::LENGTH: {
            const int count = ComponentCount(program.registerTypes[static_cast<size_t>(instruction.srcRegisters[0])]);
            const glm::vec4 source = read(instruction.srcRegisters[0]);
            float lengthValue = std::abs(source.x);
            if (count == 2) {
                lengthValue = glm::length(glm::vec2(source));
            } else if (count >= 3) {
                lengthValue = glm::length(glm::vec3(source));
            }
            value = glm::vec4(lengthValue, 0.0f, 0.0f, 0.0f);
            break;
        }
        case MaterialOpcode::POWER:
            value = glm::pow(glm::max(read(instruction.srcRegisters[0]), glm::vec4(0.0f)),
                             glm::max(read(instruction.srcRegisters[1]), glm::vec4(0.0f)));
            break;
        case MaterialOpcode::LERP:
            value = glm::mix(read(instruction.srcRegisters[0]), read(instruction.srcRegisters[1]), read(instruction.srcRegisters[2]));
            break;
        case MaterialOpcode::APPEND: {
            value = glm::vec4(0.0f);
            int writtenComponents = 0;
            for (int registerIndex : instruction.srcRegisters) {
                if (registerIndex < 0 || writtenComponents >= instruction.componentCount) {
                    continue;
                }
                value[writtenComponents++] = read(registerIndex).x;
            }
            break;
        }
        case MaterialOpcode::SWIZZLE: {
            const glm::vec4 source = read(instruction.srcRegisters[0]);
            value = glm::vec4(0.0f);
            for (int componentIndex = 0; componentIndex < instruction.componentCount; componentIndex++) {
                value[componentIndex] = source[instruction.swizzle[static_cast<size_t>(componentIndex)]];
            }
            break;
        }
        case MaterialOpcode::SAMPLE_TEXTURE: {
            Pixel sampled = Pixel{255, 255, 255, 255};
            if (samplers != nullptr && instruction.samplerIndex >= 0 && instruction.samplerIndex < static_cast<int>(samplers->size())) {
                const ResolvedMaterialSampler& sampler = (*samplers)[static_cast<size_t>(instruction.samplerIndex)];
                const glm::vec2 uv = glm::vec2(read(instruction.srcRegisters[0]));
                sampled = sampler.filter == MaterialFilterMode::LINEAR ? SampleLinear(sampler, uv) : SampleNearest(sampler, uv);
            }
            value = PixelToUnitVec4(sampled);
            break;
        }
        }

        registers[static_cast<size_t>(instruction.dstRegister)] = TruncateValue(value, instruction.resultType);
    }
}

} // namespace

bool EvaluateMaterialVertexStage(const CompiledMaterialTemplate& material,
                                 const std::vector<glm::vec4>& parameterValues,
                                 const MaterialVertexStageInput& input,
                                 MaterialVertexStageOutput& output) {
    std::vector<glm::vec4> registers;
    ExecuteProgram(material.vertexProgram, parameterValues, nullptr, input, registers);
    const auto read = [&](int registerIndex, const glm::vec4& fallback) -> glm::vec4 {
        return registerIndex >= 0 && registerIndex < static_cast<int>(registers.size()) ? registers[static_cast<size_t>(registerIndex)] : fallback;
    };

    output.positionOS = read(material.vertexOutputs.positionRegister, input.positionOS);
    output.normalOS = glm::vec3(read(material.vertexOutputs.normalRegister, glm::vec4(input.normalOS, 0.0f)));
    output.uv0 = glm::vec2(read(material.vertexOutputs.uvRegister, glm::vec4(input.uv0, 0.0f, 0.0f)));
    output.color0 = read(material.vertexOutputs.colorRegister, input.color0);
    for (size_t varyingIndex = 0; varyingIndex < output.varyings.size(); varyingIndex++) {
        output.varyings[varyingIndex] = read(material.vertexOutputs.varyingRegisters[varyingIndex], glm::vec4(0.0f));
    }
    return true;
}

MaterialFragmentStageOutput EvaluateMaterialFragmentStage(const CompiledMaterialTemplate& material,
                                                          const std::vector<glm::vec4>& parameterValues,
                                                          const std::vector<ResolvedMaterialSampler>& samplers,
                                                          const MaterialFragmentStageInput& input) {
    std::vector<glm::vec4> registers;
    ExecuteProgram(material.fragmentProgram, parameterValues, &samplers, input, registers);
    const auto read = [&](int registerIndex, const glm::vec4& fallback) -> glm::vec4 {
        return registerIndex >= 0 && registerIndex < static_cast<int>(registers.size()) ? registers[static_cast<size_t>(registerIndex)] : fallback;
    };

    MaterialFragmentStageOutput output{};
    output.baseColor = read(material.fragmentOutputs.baseColorRegister, glm::vec4(1.0f));
    output.emissive = glm::vec3(read(material.fragmentOutputs.emissiveRegister, glm::vec4(0.0f)));
    output.alpha = read(material.fragmentOutputs.alphaRegister, glm::vec4(output.baseColor.a, 0.0f, 0.0f, 0.0f)).x;
    output.ambientStrength = read(material.fragmentOutputs.ambientStrengthRegister, glm::vec4(0.3f, 0.0f, 0.0f, 0.0f)).x;
    output.specularStrength = read(material.fragmentOutputs.specularStrengthRegister, glm::vec4(0.3f, 0.0f, 0.0f, 0.0f)).x;
    output.shininess = read(material.fragmentOutputs.shininessRegister, glm::vec4(32.0f, 0.0f, 0.0f, 0.0f)).x;
    return output;
}

} // namespace RetroRenderer
