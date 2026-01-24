#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace RetroRenderer {
enum class CameraType {
    PERSPECTIVE,
    ORTHOGRAPHIC,
};

class Camera {
  public:
    Camera() = default;
    ~Camera() = default;

    void UpdateViewMatrix();

    // Vectors
    glm::vec3 m_Position = {0.0f, 0.0f, 3.0f};
    const glm::vec3 m_Up = {0.0f, 1.0f, 0.0f};
    glm::vec3 m_Direction = {0.0f, 0.0f, -1.0f};
    glm::vec3 m_EulerRotation = {0.0f, -90.0f, 0.0f};

    // Settings
    CameraType m_Type = CameraType::PERSPECTIVE;
    float m_Fov = 90.0f;
    float m_Near = 0.1f;
    float m_Far = 100.0f;
    float m_OrthoSize = 10.0f;
    float m_AspectRatio = 1.0f;

    // Matrices
    glm::mat4 m_ViewMat = glm::lookAt(m_Position, m_Position + m_Direction, m_Up);
    glm::mat4 m_ProjMat = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_Near, m_Far);

  private:
};

} // namespace RetroRenderer
