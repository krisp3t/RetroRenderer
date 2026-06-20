#pragma once

#include "MaterialTypes.h"
#include <array>
#include <vector>

namespace RetroRenderer {

struct MaterialVertexStageInput {
    glm::vec4 positionOS = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 normalOS = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 uv0 = glm::vec2(0.0f);
    glm::vec4 color0 = glm::vec4(1.0f);
    float time = 0.0f;
};

struct MaterialVertexStageOutput {
    glm::vec4 positionOS = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 normalOS = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 uv0 = glm::vec2(0.0f);
    glm::vec4 color0 = glm::vec4(1.0f);
    std::array<glm::vec4, 4> varyings = {
        glm::vec4(0.0f),
        glm::vec4(0.0f),
        glm::vec4(0.0f),
        glm::vec4(0.0f),
    };
};

struct MaterialFragmentStageInput {
    glm::vec3 worldPosition = glm::vec3(0.0f);
    glm::vec3 normalWS = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 uv0 = glm::vec2(0.0f);
    glm::vec4 color0 = glm::vec4(1.0f);
    glm::vec3 viewDirWS = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 screenUV = glm::vec2(0.0f);
    std::array<glm::vec4, 4> varyings = {
        glm::vec4(0.0f),
        glm::vec4(0.0f),
        glm::vec4(0.0f),
        glm::vec4(0.0f),
    };
};

struct MaterialFragmentStageOutput {
    glm::vec4 baseColor = glm::vec4(1.0f);
    glm::vec3 emissive = glm::vec3(0.0f);
    float alpha = 1.0f;
    float ambientStrength = 0.3f;
    float specularStrength = 0.3f;
    float shininess = 32.0f;
};

struct ResolvedMaterialSampler {
    const Texture* texture = nullptr;
    MaterialFilterMode filter = MaterialFilterMode::LINEAR;
    MaterialWrapMode wrapU = MaterialWrapMode::REPEAT;
    MaterialWrapMode wrapV = MaterialWrapMode::REPEAT;
};

bool EvaluateMaterialVertexStage(const CompiledMaterialTemplate& material,
                                 const std::vector<glm::vec4>& parameterValues,
                                 const MaterialVertexStageInput& input,
                                 MaterialVertexStageOutput& output);

MaterialFragmentStageOutput EvaluateMaterialFragmentStage(const CompiledMaterialTemplate& material,
                                                          const std::vector<glm::vec4>& parameterValues,
                                                          const std::vector<ResolvedMaterialSampler>& samplers,
                                                          const MaterialFragmentStageInput& input);

} // namespace RetroRenderer
