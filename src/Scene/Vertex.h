#pragma once
#include <glm/glm.hpp>

namespace RetroRenderer
{
    struct Vertex
    {
        glm::vec4 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        glm::vec3 color;
    };
}