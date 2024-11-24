#pragma once
#include <string>
#include <vector>
#include "Mesh.h"

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
    private:
        bool ProcessNode(aiNode* node, const aiScene* scene);
        Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
        std::vector<unsigned int> m_Textures;
        std::vector<unsigned int> m_Materials;
        std::vector<Mesh> m_Meshes;
	};
};
