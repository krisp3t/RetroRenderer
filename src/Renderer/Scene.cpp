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
		const aiScene *scene = importer.ReadFile(path.c_str(),
                                                 aiProcess_Triangulate |
                                                 aiProcess_FlipUVs);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOGE("assimp: Failed to load scene: %s", importer.GetErrorString());
			return;
		}
        LOGD("assimp: Successfully loaded scene into assimp: %s", path.c_str());

        if (ProcessNode(scene->mRootNode, scene))
        {
            LOGI("Successfully processed scene: %s (%d nodes)", scene->mRootNode->mName.C_Str());
        }
	}

    /**
     * Recursively process each node in the scene
     * @param node
     * @param scene
     * @return true if successful
     */
    bool Scene::ProcessNode(aiNode *node, const aiScene *scene)
    {
        if (!node)
        {
            LOGW("assimp: Node is null");
            return false;
        }

        /*
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(ProcessMesh(mesh, scene));
        }
        */

        for (size_t i = 0; i < node->mNumChildren; i++)
        {
            // push children
            ProcessNode(node->mChildren[i], scene);
        }


        return true;
    }

    /*
    Mesh Scene::ProcessMesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;

            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        return Mesh(vertices, indices);
    }

    void Scene::Draw(Shader shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
        {
            meshes[i].Draw(shader);
        }
    }
     */
}