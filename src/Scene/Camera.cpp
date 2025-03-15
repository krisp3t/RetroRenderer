#include "Camera.h"
#include "../Engine.h"

namespace RetroRenderer
{
    void Camera::UpdateViewMatrix()
    {
		auto const& p_config = Engine::Get().GetConfig();
		aspectRatio = p_config->window.size.x / p_config->window.size.y;

        eulerRotation.x = glm::clamp(eulerRotation.x, -89.0f, 89.0f);
        eulerRotation.y = glm::mod(eulerRotation.y, 360.0f);
        eulerRotation.z = glm::mod(eulerRotation.z, 360.0f);

        direction.x = cos(glm::radians(eulerRotation.y)) * cos(glm::radians(eulerRotation.x));
        direction.y = sin(glm::radians(eulerRotation.x));
        direction.z = sin(glm::radians(eulerRotation.y)) * cos(glm::radians(eulerRotation.x));
        direction = glm::normalize(direction);

        viewMat = glm::lookAt(position, position + direction, up);

        if (type == CameraType::PERSPECTIVE)
        {
            projMat = glm::perspective(glm::radians(fov), aspectRatio, near, far);
        } else
        {
            projMat = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near, far);
        }
    }
}
