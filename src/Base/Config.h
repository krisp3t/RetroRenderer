#pragma once
#include <glm/glm.hpp>
#include <imgui.h>

struct Config
{
    enum AAType
    {
        AAType_NONE,
        AAType_MSAA,
        AAType_FXAA,
    };

    enum CameraType
    {
        CameraType_PERSPECTIVE,
        CameraType_ORTHOGRAPHIC,
    };

    struct CameraSettings
    {
        CameraType type = CameraType_PERSPECTIVE;
        float fov = 45.0f;
        float near = 0.1f;
        float far = 100.0f;
        float orthoSize = 10.0f;
        glm::vec3 position = {0.0f, 0.0f, 5.0f};
    };

    struct RendererSettings
    {
        AAType aaType = AAType_NONE;
        bool wireframe = false;
        ImVec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
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
};
