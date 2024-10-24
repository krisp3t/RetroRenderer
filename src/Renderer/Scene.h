#pragma once
#include <string>


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
	};
};
