#pragma once
#include <SDL.h>
#include "../../Scene/Scene.h"
#include "../Buffer.h"
#include "../../Scene/Camera.h"

namespace RetroRenderer
{
    class SWRenderer
    {
    public:
        SWRenderer() = default;
        ~SWRenderer() = default;
        bool Init(int w, int h);
        void Destroy();
        void SetActiveCamera(const Camera &camera);
        void DrawFrame(Scene &scene);
        Buffer<Uint32> &GetRenderTarget();
    private:
        Buffer<Uint32> *p_FrameBuffer = nullptr;
        Camera *p_Camera = nullptr;
    };

}