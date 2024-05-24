#include "resourcemanager.h"


namespace MiniRenderer
{
	ResourceManager::ResourceManager()
	{
	}

	ResourceManager::~ResourceManager()
	{
	}

	ResourceManager& ResourceManager::get()
	{
		static ResourceManager sInstance;
		return sInstance;
	}

	void ResourceManager::set_settings(std::unique_ptr<Settings> s) {
		mSettings = std::move(s);
	}
	void ResourceManager::set_model(std::unique_ptr<Model> m) {
		mModel = std::move(m);
	}
	std::unique_ptr<Settings>& ResourceManager::get_settings() {
		return mSettings;
	}
	std::unique_ptr<Model>& ResourceManager::get_model() {
		return mModel;
	}
}	

