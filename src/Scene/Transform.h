#pragma once
#include <glm/glm.hpp>

namespace RetroRenderer
{
	struct Transform
	{
		// Local space information
		glm::vec3 m_Position = glm::vec3(0.0f);
		glm::vec3 m_EulerRotation = glm::vec3(0.0f); // in degrees
		glm::quat m_QuaternionRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 m_Scale = glm::vec3(1.0f);

		glm::mat4 m_ModelMatrix = glm::mat4(1.0f);

		bool m_IsDirty = true;

		void SetPosition(const glm::vec3& position);
		void SetEulerRotation(const glm::vec3& eulerRotation);
		void SetQuaternionRotation(const glm::quat& quaternionRotation);
		void SetScale(const glm::vec3& scale);

		glm::mat4& GetModelMatrix()
		{
			if (m_IsDirty)
			{
				m_ModelMatrix = glm::mat4(1.0f);
				// TRS matrix
				m_ModelMatrix = glm::translate(m_ModelMatrix, m_Position);
				m_ModelMatrix = m_ModelMatrix * glm::toMat4(m_QuaternionRotation);
				m_ModelMatrix = glm::scale(m_ModelMatrix, m_Scale);
				m_IsDirty = false;
			}
			return m_ModelMatrix;
	};
}