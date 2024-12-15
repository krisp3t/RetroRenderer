#pragma once

#include <glad/glad.h>
#include "Buffer.h"
#include <SDL.h>

namespace RetroRenderer
{
    class IRenderer
    {
    public:
        IRenderer() = default;

        virtual ~IRenderer() = default;

        // virtual bool Init(int w, int h) = 0;
        virtual void Destroy() = 0;

        virtual void BeforeFrame(Uint32 clearColor) = 0; // Clear screen / framebuffer texture

        virtual GLuint EndFrame() = 0; // Copy to framebuffer texture

        virtual void SetActiveCamera(const Camera &camera) = 0;

        virtual void DrawTriangularMesh(const Model *model) = 0;

    };
}