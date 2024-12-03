#pragma once
#include <string>
#include <vector>
#include <queue>
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
		Scene(const std::string& path);

		void Load(const char* path);
		void Unload();
		void Render();
        [[nodiscard]] std::queue<Model*>& GetVisibleModels();
        void FrustumCull(const Camera& camera);
    private:
        // TODO: unneeded?
        std::vector<unsigned int> m_Textures;
        std::vector<unsigned int> m_Materials;
        std::vector<Mesh> m_Meshes;

        std::vector<Model*> m_Models;
        std::queue<Model*> m_VisibleModels;

        bool ProcessNode(aiNode* node, const aiScene* scene);
        Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

    };
};