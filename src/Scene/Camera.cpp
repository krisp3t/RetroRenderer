#include "Camera.h"

namespace RetroRenderer
{
	void Camera::Update()
	{
		viewMat = glm::lookAt(position, target, up);

		if (type == CameraType::PERSPECTIVE)
		{
			constexpr float aspectRatio = 800.0f / 600.0f; // TODO: replace!!
			projMat = glm::perspective(glm::radians(fov), aspectRatio, near, far);
		}
		else
		{
			projMat = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near, far);
		}
	}
