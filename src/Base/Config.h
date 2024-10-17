#pragma once
#include <glm/glm.hpp>

class Config
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

};
