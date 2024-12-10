#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace RetroRenderer
{
	enum class CameraType
	{
		PERSPECTIVE,
		ORTHOGRAPHIC,
	};

	class Camera
	{
	public:
		// Matrices
		glm::mat4 viewMat = glm::lookAt(position, position + direction, up);
		glm::mat4 projMat = glm::perspective(glm::radians(fov), aspectRatio, near, far);
		glm::vec3 position = { 0.0f, 0.0f, 3.0f };
		const glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 direction = { 0.0f, 0.0f, -1.0f };

		// Settings
		CameraType type = CameraType::PERSPECTIVE;
		float fov = 90.0f;
		float near = 0.1f;
		float far = 100.0f;
		float orthoSize = 10.0f;

		Camera() = default;
		void UpdateViewMatrix();
	private:
		const float aspectRatio = 800.0f / 600.0f; // TODO: replace!!
	};

}
