#pragma once

#include <glm/vec3.hpp>

namespace RetroRenderer {

struct SoftwareMaterialState {
    glm::vec3 lightColor = glm::vec3(1.0f);
    float ambientStrength = 0.0f;
    float specularStrength = 0.0f;
    float shininess = 32.0f;
    bool enablePhong = false;
    bool useVertexColor = false;
};

} // namespace RetroRenderer
