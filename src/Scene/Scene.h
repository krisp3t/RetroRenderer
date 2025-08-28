#pragma once

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include "Mesh.h"
#include "Camera.h"
#include "Model.h"

struct aiNode;
struct aiScene;
struct aiMesh;

namespace RetroRenderer
{
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        bool Load(const uint8_t* data, const size_t size);
        bool Load(const std::string &path);
        void FrustumCull(const Camera &camera);
        [[nodiscard]] std::vector<int> &GetVisibleModels();
        [[nodiscard]] const glm::mat4& GetModelWorldTransform(int index) const;
        void MarkDirtyModel(int index);

        std::vector<Model> m_Models;
    private:
        bool ProcessScene(const aiScene* scene);
        bool ProcessNode(aiNode *node, const aiScene *scene);
        bool ProcessNode(aiNode *node, const aiScene *scene, int parentIndex);
        void ProcessMesh(aiMesh *mesh, const aiScene *scene, std::vector<Mesh> &meshes, const aiString& modelName);

        std::vector<int> m_VisibleModels;
    };
};
