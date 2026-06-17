#pragma once

#include "ShaderHandle.h"
#include <string>

namespace RetroRenderer {

class IRenderInvalidationSink {
  public:
    virtual ~IRenderInvalidationSink() = default;

    virtual void OnSceneMutated() = 0;
    virtual void OnTextureMutated() = 0;
};

class IShaderCompiler {
  public:
    virtual ~IShaderCompiler() = default;

    [[nodiscard]] virtual ShaderHandle CompileShaders(const std::string& vertexCode,
                                                      const std::string& fragmentCode) = 0;
};

} // namespace RetroRenderer
