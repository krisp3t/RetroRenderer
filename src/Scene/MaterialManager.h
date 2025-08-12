#pragma once
#include <string>
#include <vector>
#include <glm/vec3.hpp>
#ifdef __ANDROID__
#include <GLES3/gl3.h> // For OpenGL ES 3.0
#else
#include <glad/glad.h>
#endif

namespace RetroRenderer
{

class MaterialManager
{
public:
    struct Material
    {
        GLuint shaderProgram;
        GLuint textureID;
        std::string name;
        std::string texturePath;

        float ambientStrength = 0.3f;
        float specularStrength = 0.3f;
        float shininess = 32.0f;
        glm::vec3 lightColor = glm::vec3(1.0f);
        glm::vec3 objectColor = glm::vec3(1.0f);
    };
    MaterialManager() = default;
    ~MaterialManager() = default;
    bool Init();
    void LoadTexture(const std::string &path);
    void RenderUI();
private:
    std::vector<Material> m_Materials;
    int m_CurrentMaterialIndex = 0;
    char m_TexturePathBuffer[256] = "";
};

}
