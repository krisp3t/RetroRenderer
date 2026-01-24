#pragma once

namespace RetroRenderer {
struct IShader {
    virtual ~IShader() = default;
    virtual void VertexShader() = 0;
    virtual void FragmentShader() = 0;
};
} // namespace RetroRenderer