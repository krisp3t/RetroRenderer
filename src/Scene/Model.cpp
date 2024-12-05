#include "Model.h"

namespace RetroRenderer
{
	Model::Model(Mesh& mesh)
		: m_Mesh(&mesh)
	{
	}

	Mesh& Model::GetMesh() const
	{
		return *m_Mesh;
	}
}