#pragma once
#include <SDL.h>
#include "../../Scene/Scene.h"
#include "../Buffer.h"

namespace RetroRenderer
{
    class SWRenderer
    {
    public:
        SWRenderer() = default;
        ~SWRenderer() = default;
        bool Init(int w, int h);
        void Destroy();

        void DrawFrame(Scene &scene);
        Buffer<Uint32> &GetRenderTarget();
    private:
        Buffer<Uint32> *p_FrameBuffer = nullptr;
    };

}