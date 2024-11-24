#pragma once
#include <glm/glm.hpp>

namespace RetroRenderer
{

struct Camera
{
    Camera() = default;
    glm::mat4 viewMat;
    glm::mat4 projMat;
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    bool enablePerspective = true;
};

}
