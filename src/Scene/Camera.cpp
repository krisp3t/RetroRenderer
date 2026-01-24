#include "Camera.h"
#include "../Engine.h"

namespace RetroRenderer {
void Camera::UpdateViewMatrix() {
    auto const& p_config = Engine::Get().GetConfig();
    m_AspectRatio =
        static_cast<float>(p_config->renderer.resolution.x) / static_cast<float>(p_config->renderer.resolution.y);

    m_EulerRotation.x = glm::clamp(m_EulerRotation.x, -89.0f, 89.0f);
    m_EulerRotation.y = glm::mod(m_EulerRotation.y, 360.0f);
    m_EulerRotation.z = glm::mod(m_EulerRotation.z, 360.0f);

    m_Direction.x = cos(glm::radians(m_EulerRotation.y)) * cos(glm::radians(m_EulerRotation.x));
    m_Direction.y = sin(glm::radians(m_EulerRotation.x));
    m_Direction.z = sin(glm::radians(m_EulerRotation.y)) * cos(glm::radians(m_EulerRotation.x));
    m_Direction = glm::normalize(m_Direction);

    m_ViewMat = glm::lookAt(m_Position, m_Position + m_Direction, m_Up);

    if (m_Type == CameraType::PERSPECTIVE) {
        m_ProjMat = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_Near, m_Far);
    } else {
        m_ProjMat = glm::ortho(-m_OrthoSize, m_OrthoSize, -m_OrthoSize, m_OrthoSize, m_Near, m_Far);
    }
}
} // namespace RetroRenderer
