#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Scene.h"
#include "../Base/Logger.h"
#include "../Renderer/Vertex.h"


namespace RetroRenderer
{
	bool Scene::Load(const std::string& path)
	{
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(path.c_str(),
                                                 aiProcess_Triangulate |
                                                 aiProcess_FlipUVs);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGE("assimp: Failed to load scene: %s", importer.GetErrorString());
            return false;
		}

        if (ProcessNode(scene->mRootNode, scene))
        {
            // LOGI("Successfully processed scene: %s (%d meshes)", scene->mRootNode->mName.C_Str(), m_Meshes.size());
			LOGI("Successfully processed scene: %s", scene->mRootNode->mName.C_Str());
        }
        return true;
	}

    /**
     * Recursively process each node in the scene
     * @param node
     * @param scene
     * @return true if successful
     */
    bool Scene::ProcessNode(aiNode *node, const aiScene *scene, const Model* parent)
    {
        if (!node)
        {
            LOGE("assimp: Node is null");
            return false;
        }
		if (parent) {
			auto& parentName = parent->GetName();
			LOGD("Processing node: %s (%d meshes), parent: %s",
				node->mName.C_Str(),
				node->mNumMeshes,
				parentName.c_str());
		}
		else 
        {
			LOGD("Processing node: %s (%d meshes), parent: none",
				node->mName.C_Str(),
				node->mNumMeshes);
		}
        Model* model = nullptr;
		std::vector<Mesh*> modelMeshes;

        // Process meshes
        for (size_t i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			if (mesh->mNumVertices == 0 || mesh->mNumFaces == 0)
			{
				continue;
			}
            auto* processedMesh = ProcessMesh(mesh, scene);
            // m_Meshes.push_back(processedMesh); // add to all scene meshes
			modelMeshes.push_back(processedMesh); // add to current model
        }

		model = new Model(std::move(modelMeshes)); // TODO: memory leak
		model->SetName(node->mName);
        if (parent)
        {
			model->SetParentTransform(parent->GetTransform());
        }
        model->SetTransform(node->mTransformation);
		m_Models.push_back(model);

        // Recursively process children
        for (size_t i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene, model);
        }

        return true;
    }

    /**
	* Process a root node in the scene
	 * @param node
	 * @param scene
	 * @return true if successful
	 */
    bool Scene::ProcessNode(aiNode* node, const aiScene* scene)
    {
		return ProcessNode(node, scene, nullptr);
    }

    Mesh* Scene::ProcessMesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (size_t i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;

            if (mesh->HasPositions())
            {
                vertex.position.x = mesh->mVertices[i].x;
                vertex.position.y = mesh->mVertices[i].y;
                vertex.position.z = mesh->mVertices[i].z;
				vertex.position.w = 1.0f;
            }
            if (mesh->HasNormals())
            {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
                // TODO: w?
            }
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.texCoords = vec;
            }
            else
            {
                vertex.texCoords = glm::vec2(0.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }

        for (size_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            assert (face.mNumIndices == 3 && "Face must have 3 indices");
            for (size_t j = 0; j < 3; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            // TODO: process material
        }

        auto *meshObj = new Mesh(std::move(vertices), std::move(indices));
        meshObj->m_numVertices = mesh->mNumVertices;
        meshObj->m_numFaces = mesh->mNumFaces;
        return meshObj;
    }

    std::queue<Model*>& Scene::GetVisibleModels()
    {
        return m_VisibleModels;
    }

    void Scene::FrustumCull(const Camera &camera)
    {
        for (auto &model : m_Models)
        {
            bool visible = true; // TODO: add visibility check
            if (visible)
            {
                m_VisibleModels.push(model);
            }
        }
    }
}