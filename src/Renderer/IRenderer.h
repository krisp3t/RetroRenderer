#pragma once

#include "../Scene/Light.h"
#include "ShaderHandle.h"
#include <string>
#include <vector>

namespace RetroRenderer {
class Camera;

class Model;

struct Color;

class IRenderer {
  public:
    IRenderer() = default;

    virtual ~IRenderer() = default;

    // virtual bool Init(int w, int h) = 0;
    virtual void Destroy() {
    }

    virtual void BeforeFrame(const Color& clearColor) = 0; // Clear screen / framebuffer texture

    virtual void EndFrame() = 0;

    virtual void SetActiveCamera(const Camera& camera) = 0;
    virtual void SetSceneLights(const std::vector<LightSnapshot>& lights) = 0;

    virtual void DrawTriangularMesh(const Model* model) = 0;

    virtual void DrawSkybox() = 0;

    virtual void DrawGridGizmo() {
    }

    [[nodiscard]] virtual ShaderHandle CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) = 0;
};
} // namespace RetroRenderer
