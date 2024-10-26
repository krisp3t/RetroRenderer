#pragma once
#include "../Scene.h"

namespace RetroRenderer
{
    class SWRenderer
    {
    public:
        SWRenderer() = default;
        ~SWRenderer() = default;
        bool Init();
        void Destroy();

        void DrawFrame(Scene &scene);
    };
}