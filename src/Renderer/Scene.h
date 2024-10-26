#pragma once
#include <string>

struct aiNode;
struct aiScene;

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
	};
};
