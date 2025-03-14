#pragma once

namespace RetroRenderer
{
    class Camera;

    class Model;

    struct Color;

    class IRenderer
    {
    public:
        IRenderer() = default;

        virtual ~IRenderer() = default;

        // virtual bool Init(int w, int h) = 0;
        virtual void Destroy() = 0;

        virtual void BeforeFrame(const Color &clearColor) = 0; // Clear screen / framebuffer texture

        virtual GLuint EndFrame() = 0; // Copy to framebuffer texture

        virtual void SetActiveCamera(const Camera &camera) = 0;

        virtual void DrawTriangularMesh(const Model *model) = 0;

    };
}