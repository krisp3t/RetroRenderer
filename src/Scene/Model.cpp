#include "Model.h"
#include "Scene.h"
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace RetroRenderer {
Model::Model() {
    m_Meshes = std::vector<Mesh>();
}

void Model::Init(Scene *scene, const std::string &name, const aiMatrix4x4 &localMatrix) {
    p_Scene = scene;
    m_Name = name;
    m_LocalMatrix = AssimpToGlmMatrix(localMatrix);
}

const std::vector<Mesh> &Model::GetMeshes() const {
    return m_Meshes;
}

const std::string &Model::GetName() const {
    return m_Name;
}

void Model::SetName(const aiString &name) {
    m_Name = name.C_Str();
}

glm::mat4 Model::AssimpToGlmMatrix(const aiMatrix4x4 &mat) {
    return glm::mat4(mat[0][0], mat[1][0], mat[2][0], mat[3][0], mat[0][1], mat[1][1], mat[2][1], mat[3][1], mat[0][2],
                     mat[1][2], mat[2][2], mat[3][2], mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
}

void Model::MarkDirty() {
    RecomputeWorldMatrix();
    assert(p_Scene != nullptr && "Scene hasn't been assigned to model");
    for (const auto &childIx : m_Children) {
        p_Scene->MarkDirtyModel(childIx);
    }
}

void Model::SetParent(int parent) {
    m_Parent = parent;
    MarkDirty();
}

void Model::SetLocalTransform(const aiMatrix4x4 &mat) {
    m_LocalMatrix = AssimpToGlmMatrix(mat);
    MarkDirty();
}

const glm::mat4 &Model::GetWorldTransform() const {
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

void Model::GetLocalTRS(glm::vec3 &outTranslation, glm::vec3 &outRotationEuler, glm::vec3 &outScale) const {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat rotation;

    glm::decompose(m_LocalMatrix, outScale, rotation, outTranslation, skew, perspective);
    outRotationEuler = glm::degrees(glm::eulerAngles(rotation));
}

void Model::GetWorldTRS(glm::vec3 &outTranslation, glm::vec3 &outRotationEuler, glm::vec3 &outScale) const {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat rotation;

    glm::decompose(m_WorldMatrix, outScale, rotation, outTranslation, skew, perspective);
    outRotationEuler = glm::degrees(glm::eulerAngles(rotation));
}
} // namespace RetroRenderer
