#include "Model.h"
#include "Scene.h"
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace RetroRenderer {
Model::Model() {
    m_Meshes = std::vector<Mesh>();
}

void Model::Init(Scene* scene, const std::string& name, const glm::mat4& localMatrix) {
    p_Scene = scene;
    m_Name = name;
    m_LocalMatrix = localMatrix;
}

const std::vector<Mesh>& Model::GetMeshes() const {
    return m_Meshes;
}

const std::string& Model::GetName() const {
    return m_Name;
}

void Model::SetName(const std::string& name) {
    m_Name = name;
}

void Model::MarkDirty() {
    RecomputeWorldMatrix();
    assert(p_Scene != nullptr && "Scene hasn't been assigned to model");
    for (const auto& childIx : m_Children) {
        p_Scene->MarkDirtyModel(childIx);
    }
}

void Model::SetParent(int parent) {
    m_Parent = parent;
    MarkDirty();
}

void Model::SetLocalTransform(const glm::mat4& mat) {
    m_LocalMatrix = mat;
    MarkDirty();
}

const glm::mat4& Model::GetWorldTransform() const {
    return m_WorldMatrix;
}

void Model::RecomputeWorldMatrix() {
    auto parentWorld = glm::mat4(1.0f);
    assert(p_Scene != nullptr && "Scene hasn't been assigned to model");
    if (m_Parent.has_value()) {
        parentWorld = p_Scene->GetModelWorldTransform(m_Parent.value());
    }
    m_WorldMatrix = parentWorld * m_LocalMatrix;
}

void Model::GetLocalTRS(glm::vec3& outTranslation, glm::vec3& outRotationEuler, glm::vec3& outScale) const {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat rotation;

    glm::decompose(m_LocalMatrix, outScale, rotation, outTranslation, skew, perspective);
    outRotationEuler = glm::degrees(glm::eulerAngles(rotation));
}

void Model::GetWorldTRS(glm::vec3& outTranslation, glm::vec3& outRotationEuler, glm::vec3& outScale) const {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat rotation;

    glm::decompose(m_WorldMatrix, outScale, rotation, outTranslation, skew, perspective);
    outRotationEuler = glm::degrees(glm::eulerAngles(rotation));
}
} // namespace RetroRenderer
