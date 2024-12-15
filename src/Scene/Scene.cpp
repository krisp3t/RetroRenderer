#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Scene.h"
#include "../Base/Logger.h"
#include "../Renderer/Vertex.h"


namespace RetroRenderer
{
    bool Scene::Load(const std::string &path)
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
     * Recursively process each node in the scene. One node will map to one model, which can have multiple meshes.
     * @param node
     * @param scene
     * @return true if successful
     */
    bool Scene::ProcessNode(aiNode *node, const aiScene *scene, int parentIndex)
    {
        if (!node)
        {
            LOGE("assimp: Node is null");
            return false;
        }
        LOGD("Processing node: %s (%d meshes), parent index: %d",
             node->mName.C_Str(),
             node->mNumMeshes,
             parentIndex);
        /*
        if (parent)
        {
            auto &parentName = parent->GetName();
            LOGD("Processing node: %s (%d meshes), parent: %s",
                 node->mName.C_Str(),
                 node->mNumMeshes,
                 parentName.c_str());
        } else
        {
            LOGD("Processing node: %s (%d meshes), parent: none",
                 node->mName.C_Str(),
                 node->mNumMeshes);
        }
         */

        // Create a model for this aiNode
        Model newModel{};
        newModel.SetName(node->mName);
        newModel.m_Parent = parentIndex;
        newModel.SetTransform(node->mTransformation);

        // Process all meshes for this model
        for (size_t i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            if (mesh->mNumVertices > 0 && mesh->mNumFaces > 0)
            {
                ProcessMesh(mesh, scene, newModel.m_Meshes);
            }
        }
        int currentNodeIndex = m_Models.size();
        if (parentIndex != -1)
        {
            newModel.SetParentTransform(m_Models[parentIndex].GetTransform());
            m_Models[parentIndex].m_Children.push_back(currentNodeIndex);
        }
        m_Models.push_back(std::move(newModel));

        // Recursively process children
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene, currentNodeIndex);
        }

        return true;
    }

    /**
	* Process a root node in the scene
	 * @param node
	 * @param scene
	 * @return true if successful
	 */
    bool Scene::ProcessNode(aiNode *node, const aiScene *scene)
    {
        return ProcessNode(node, scene, -1);
    }

    void Scene::ProcessMesh(aiMesh *mesh, const aiScene *scene, std::vector<Mesh> &meshes)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        // TODO: add textures
        // std::vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
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
            } else
            {
                vertex.texCoords = glm::vec2(0.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            assert (face.mNumIndices == 3 && "Face must have 3 indices");
            for (unsigned int j = 0; j < 3; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            // TODO: process material
        }

        meshes.emplace_back(std::move(vertices), std::move(indices));
    }

    std::vector<int> &Scene::GetVisibleModels()
    {
        return m_VisibleModels;
    }

    void Scene::FrustumCull(const Camera &camera)
    {
        // TODO: actually implement frustum culling
        m_VisibleModels.clear();
        m_VisibleModels.reserve(m_Models.size());
        int i = 0;
        for (int i = 0; i < m_Models.size(); i++)
        {
            m_VisibleModels.push_back(i);
        }
    }
}