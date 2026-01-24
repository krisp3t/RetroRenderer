#pragma once

#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../Base/Handle.h"
#include "Mesh.h"

namespace RetroRenderer {
class Scene;

class Model {
  public:
    Model();
    ~Model() = default;
    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;
    Model(Model &&) noexcept = default;
    Model &operator=(Model &&) noexcept = default;

    void Init(Scene *scene, const std::string &name, const aiMatrix4x4 &localMatrix);
    const std::vector<Mesh> &GetMeshes() const;
    const std::string &GetName() const;
    void SetName(const aiString &name);
    void SetLocalTransform(const aiMatrix4x4 &mat);
    void SetParent(int parent);
    const glm::mat4 &GetWorldTransform() const;
    void MarkDirty();
    void GetLocalTRS(glm::vec3 &outTranslation, glm::vec3 &outRotationEuler, glm::vec3 &outScale) const;
    void GetWorldTRS(glm::vec3 &outTranslation, glm::vec3 &outRotationEuler, glm::vec3 &outScale) const;

    std::optional<int> m_Parent;
    std::vector<int> m_Children;
    std::vector<Mesh> m_Meshes;

  private:
    glm::mat4 AssimpToGlmMatrix(const aiMatrix4x4 &mat);
    void RecomputeWorldMatrix();

    Scene *p_Scene = nullptr;
    std::string m_Name;
    glm::mat4 m_WorldMatrix = glm::mat4(1.0f);
    glm::mat4 m_LocalMatrix = glm::mat4(1.0f);

    // TODO: replace with handles if needed to improve performance
};

} // namespace RetroRenderer
