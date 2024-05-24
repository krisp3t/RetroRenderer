#pragma once

#include <memory>

namespace MiniRenderer
{
	class ResourceManager
	{
	public:
		ResourceManager() = default;
		~ResourceManager() = default;
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		void set_settings(std::unique_ptr<Settings> s);
		void set_model(std::unique_ptr<Model> m);
		std::unique_ptr<Settings>& get_settings(void);
		std::unique_ptr<Model>& get_model(void);

		static ResourceManager& get();

	private:
		static ResourceManager sInstance;
		std::unique_ptr<Settings> mSettings;
		std::unique_ptr<Model> mModel;
	};
}