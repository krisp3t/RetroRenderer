#include "Camera.h"

namespace RetroRenderer
{
	Camera::Camera()
	{
		UpdateViewMatrix();
	}

	void Camera::UpdateViewMatrix()
	{
		eulerRotation.x = glm::clamp(eulerRotation.x, -89.0f, 89.0f);
		eulerRotation.y = glm::clamp(eulerRotation.y, -180.0f, 180.0f);
		eulerRotation.z = glm::clamp(eulerRotation.z, -180.0f, 180.0f);

		direction.x = cos(glm::radians(eulerRotation.y)) * cos(glm::radians(eulerRotation.x));
		direction.y = sin(glm::radians(eulerRotation.x));
		direction.z = sin(glm::radians(eulerRotation.y)) * cos(glm::radians(eulerRotation.x));
		direction = glm::normalize(direction);

		viewMat = glm::lookAt(position, position + direction, up);

		if (type == CameraType::PERSPECTIVE)
		{
			projMat = glm::perspective(glm::radians(fov), aspectRatio, near, far);
		}
		else
		{
			projMat = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near, far);
		}
	}
}
