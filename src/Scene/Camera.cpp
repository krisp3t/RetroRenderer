#include "Camera.h"

namespace RetroRenderer
{
	void Camera::UpdateViewMatrix()
	{
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
