#pragma once
#include <SDL.h>
#include <memory>
#include "../../Scene/Scene.h"
#include "../../Scene/Camera.h"
#include "../Buffer.h"
#include "Rasterizer.h"
#include "../IRenderer.h"

namespace RetroRenderer
{
	class SWRenderer : public IRenderer
    {
    public:
        SWRenderer() = default;
        ~SWRenderer() = default;
        bool Init(int w, int h);
        void Destroy();
        void SetActiveCamera(const Camera &camera);
        void DrawTriangularMesh(const Model *model);
        Buffer<Uint32> &GetRenderTarget();
    private:
        Buffer<Uint32> *p_FrameBuffer = nullptr;
        Camera *p_Camera = nullptr;
        std::unique_ptr<Rasterizer> m_Rasterizer = nullptr;
    };

}