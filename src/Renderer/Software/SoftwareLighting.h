#pragma once

#include "../MaterialRuntime.h"
#include "../MaterialTypes.h"
#include <memory>
#include <vector>

namespace RetroRenderer {

struct SoftwareMaterialState {
    std::shared_ptr<const CompiledMaterialTemplate> compiledTemplate;
    std::vector<glm::vec4> parameterValues;
    std::vector<ResolvedMaterialSampler> samplers;
    MaterialPipelineState pipelineState{};
};

} // namespace RetroRenderer
