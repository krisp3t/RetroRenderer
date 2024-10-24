#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Scene.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
	Scene::Scene(const std::string& path)
	{
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGE("assimp: Failed to load scene: %s", importer.GetErrorString());
			return;
		}
        LOGD("assimp: Successfully loaded scene: %s", path.c_str());
	}
}