#pragma once
#include <glm/glm.hpp>
#include <imgui.h>

namespace RetroRenderer
{

struct Config
{
    enum class AAType
    {
        NONE,
        MSAA,
        FXAA,
    };

    struct RendererSettings
    {
        AAType aaType = AAType::NONE;
        bool showWireframe = false;
        bool enablePerspectiveCorrect = true;
        ImVec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        int viewportResolution[2] = {1280, 720};
    };

    struct EnvironmentSettings
    {
        bool showSkybox = false;
        bool showGrid = true;
        bool showFloor = true;
        bool shadowMap = true;
    };

    struct CullSettings
    {
        bool cullFace = true;
        bool depthTest = true;
    };

    EnvironmentSettings environment;
    CullSettings cull;
    RendererSettings renderer;
    bool showConfigPanel = true;
};

}

