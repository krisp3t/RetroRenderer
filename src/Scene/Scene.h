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

        bool Load(const std::string &path);
        [[nodiscard]] std::vector<int> &GetVisibleModels();
        void FrustumCull(const Camera &camera);
        std::vector<Model> m_Models;
    private:
        std::vector<int> m_VisibleModels;

        bool ProcessNode(aiNode *node, const aiScene *scene);
        bool ProcessNode(aiNode *node, const aiScene *scene, int parentIndex);
        void ProcessMesh(aiMesh *mesh, const aiScene *scene, std::vector<Mesh> &meshes);

    };
};
