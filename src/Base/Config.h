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

    enum class CameraType
    {
        PERSPECTIVE,
        ORTHOGRAPHIC,
    };

    struct CameraSettings
    {
        CameraType type = CameraType::PERSPECTIVE;
        float fov = 45.0f;
        float near = 0.1f;
        float far = 100.0f;
        float orthoSize = 10.0f;
        glm::vec3 position = {0.0f, 0.0f, 5.0f};
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

    CameraSettings camera;
    EnvironmentSettings environment;
    CullSettings cull;
    RendererSettings renderer;
    bool showConfigPanel = true;
};

}

