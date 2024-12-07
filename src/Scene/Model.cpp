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
}