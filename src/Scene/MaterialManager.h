#pragma once
#include <string>
#include <vector>
#include <glm/vec3.hpp>

#include "Texture.h"
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
    struct ShaderProgram
    {
        GLuint id = 0;
        std::string name;
        std::string vertexPath;
        std::string fragmentPath;
        // time_t lastModified = 0;
    };

    struct Material
    {
        ShaderProgram shaderProgram;
        Texture texture;
        std::string name;

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
    void LoadTexture(const uint8_t* data, const size_t size);
    void RenderUI();
    void LoadDefaultShaders();
    Material& GetCurrentMaterial() { return m_Materials[m_CurrentMaterialIndex]; }

private:
    ShaderProgram CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);
    std::string ReadShaderFile(const std::string& path);
    void CheckShaderErrors(GLuint shader, const std::string& type);
private:
    std::vector<Material> m_Materials;
    int m_CurrentMaterialIndex = 0;
    char m_TexturePathBuffer[256] = "";
    char vertPathBuffer[256] = "";
    char fragPathBuffer[256] = "";
    int newShaderType = 0; // For UI combo box
    const char* k_supportedTextures = ".png";

};

}
