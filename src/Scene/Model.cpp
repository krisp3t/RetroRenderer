#include "Model.h"

namespace RetroRenderer
{
	Model::Model(std::vector<Mesh*> meshes)
		: m_Meshes(meshes)
	{
	}

	const std::vector<Mesh*>& Model::GetMeshes() const
	{
		return m_Meshes;
	}

	const std::string& Model::GetName() const
	{
		if (m_Name.empty())
		{
			return "";
		}
		return m_Name;
	}

	void Model::SetName(const aiString& name)
	{
		m_Name = name.C_Str();
	}

	glm::mat4 Model::AssimpToGlmMatrix(const aiMatrix4x4& mat)
	{
		return glm::mat4(
			mat[0][0], mat[1][0], mat[2][0], mat[3][0],
			mat[0][1], mat[1][1], mat[2][1], mat[3][1],
			mat[0][2], mat[1][2], mat[2][2], mat[3][2],
			mat[0][3], mat[1][3], mat[2][3], mat[3][3]
		);
	}

	void Model::SetTransform(const aiMatrix4x4& mat)
	{
		m_nodeTransform = AssimpToGlmMatrix(mat);
	}

	void Model::SetParentTransform(const glm::mat4& mat)
	{
		m_parentTransform = mat;
	}

	void Model::SetParentTransform(const aiMatrix4x4& mat)
	{
		m_parentTransform = AssimpToGlmMatrix(mat);
	}

	void Model::SetLocalTransform(const aiMatrix4x4& mat)
	{
		auto localTransform = AssimpToGlmMatrix(mat);
		m_nodeTransform = m_parentTransform * localTransform;
	}

	const glm::mat4& Model::GetTransform() const
	{
		return m_nodeTransform;
	}
}