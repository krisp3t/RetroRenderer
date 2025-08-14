#pragma once
#include <optional>
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
    struct ShaderProgram
    {
        GLuint id = 0;
        std::string name;
        std::string vertexPath;
        std::string fragmentPath;
        // time_t lastModified = 0;
    };

class MaterialManager
{
public:
    struct alignas(16) LightingParamsUBO {
        int lightingModel;   // 0 = none, 1 = Phong, 2 = Blinn-Phong, 3 = PBR
        float padding1, padding2, padding3;

        float phongAmbient;
        float phongSpecular;
        float phongShininess;
        float padding4;

        float blinnAmbient;
        float blinnSpecular;
        float blinnShininess;
        float padding5;

        float pbrMetallic;
        float pbrRoughness;
        float pbrAO;
        float padding6;
    };

    struct PhongParams
    {
        float ambientStrength = 0.3f;
        float specularStrength = 0.3f;
        float shininess = 32.0f;
    };
    struct Material
    {
        ShaderProgram shaderProgram;
        std::optional<Texture> texture;
        std::string name;
        std::optional<PhongParams> phongParams;
        glm::vec3 lightColor = glm::vec3(1.0f);
    };

    MaterialManager() = default;
    ~MaterialManager() = default;
    bool Init();
    void LoadTexture(const std::string &path);
    void LoadTexture(const uint8_t* data, const size_t size);
    void RenderUI();
    void LoadDefaultShaders();
    Material& GetCurrentMaterial() { return m_Materials[m_CurrentMaterialIndex]; }
    static ShaderProgram CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);

private:
    static std::string ReadShaderFile(const std::string& path);
    static void CheckShaderErrors(GLuint shader, const std::string& type);
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
