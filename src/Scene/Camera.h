#pragma once
#include <glm/glm.hpp>

namespace RetroRenderer
{
	class Camera
	{
		enum class CameraType
		{
			PERSPECTIVE,
			ORTHOGRAPHIC,
		};

	public:
		// Matrices
		glm::mat4 viewMat;
		glm::mat4 projMat;
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		const glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 target = { 0.0f, 0.0f, -1.0f };

		// Settings
		CameraType type = CameraType::PERSPECTIVE;
		float fov = 45.0f;
		float near = 0.1f;
		float far = 100.0f;
		float orthoSize = 10.0f;

		Camera() = default;
		void Update();
	private:
	};

}
