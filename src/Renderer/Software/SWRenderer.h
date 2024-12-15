#pragma once

#include <memory>
#include "../../Scene/Scene.h"
#include "../../Scene/Camera.h"
#include "../Buffer.h"
#include "Rasterizer.h"
#include "../IRenderer.h"
#include "../../Base/Color.h"

namespace RetroRenderer
{
    class SWRenderer : public IRenderer
    {
    public:
        SWRenderer() = default;

        ~SWRenderer() = default;

        bool Init(GLuint fbTexture, int w, int h);

        void Resize(int w, int h);

        void Destroy();

        void SetActiveCamera(const Camera &camera);

        void DrawTriangularMesh(const Model *model);

        void BeforeFrame(const Color &clearColor) override;

        GLuint EndFrame() override;

    private:
        Buffer<uint32_t> *m_FrameBuffer = nullptr;
        GLuint p_FrameBufferTexture = 0;
        Camera *p_Camera = nullptr;
        std::unique_ptr<Rasterizer> m_Rasterizer = nullptr;
    };

}