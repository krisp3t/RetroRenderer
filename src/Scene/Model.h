#pragma once
#include <glm/glm.hpp>
#include "../Base/Handle.h"
#include "Mesh.h"

namespace RetroRenderer
{

class Model
{
public:
    Model(Mesh& mesh);
    ~Model() = default;
    Mesh& GetMesh() const;
private:
	glm::mat4 m_worldMatrix = glm::mat4(1.0f);
	glm::mat4 m_localMatrix = glm::mat4(1.0f);
    Handle p_Parent;
	std::vector<Handle> m_Children;

    Mesh* m_Mesh = nullptr;
};

}
