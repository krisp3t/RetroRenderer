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

        bool Init(int w, int h);

        void Resize(int w, int h);

        void Destroy() override;

        void SetActiveCamera(const Camera &camera) override;

        void DrawTriangularMesh(const Model *model) override;

        void DrawSkybox() override;

        void BeforeFrame(const Color &clearColor) override;

        GLuint EndFrame() override;

        GLuint CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) override
        {
            // TODO: implement
            return 0;
        }

        [[nodiscard]] const Buffer<uint32_t> *GetFrameBuffer() const;

    private:
        Buffer<uint32_t> *m_FrameBuffer = nullptr;
        Camera *p_Camera = nullptr;
        std::unique_ptr<Rasterizer> m_Rasterizer = nullptr;
    };

}
