#pragma once
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <string>
#include <vector>
#include "../Base/Handle.h"
#include "Mesh.h"

namespace RetroRenderer
{

class Model
{
public:
    Model(std::vector<Mesh*> meshes);
    ~Model() = default;
    const std::vector<Mesh*>& GetMeshes() const;
    const std::string& GetName() const;
	void SetName(const aiString& name);
    void SetTransform(const aiMatrix4x4& mat);
	void SetLocalTransform(const aiMatrix4x4& mat);
	void SetParentTransform(const glm::mat4& mat);
	void SetParentTransform(const aiMatrix4x4& mat);
    const glm::mat4& GetTransform() const;
private:
    std::string m_Name;
	glm::mat4 m_parentTransform = glm::mat4(1.0f);
    glm::mat4 m_nodeTransform = glm::mat4(1.0f);

	glm::mat4 m_worldMatrix = glm::mat4(1.0f);
	glm::mat4 m_localMatrix = glm::mat4(1.0f);

    // TODO: replace with handles if needed to improve performance
    Model* p_Parent = nullptr;
	std::vector<Model*> m_Children;
	std::vector<Mesh*> m_Meshes;
	glm::mat4 AssimpToGlmMatrix(const aiMatrix4x4& mat);
};

}
