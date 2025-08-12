#include "MaterialManager.h"

#include <fstream>
#include <imgui.h>
#include <sstream>

#include "../Engine.h"
#include "KrisLogger/Logger.h"

namespace RetroRenderer
{
    bool MaterialManager::Init()
    {
        LoadDefaultShaders();
        return true;
    }

    void MaterialManager::LoadTexture(const std::string& path)
    {
    }

    void MaterialManager::LoadDefaultShaders()
    {
        Material phongTexMaterial;
        phongTexMaterial.name = "Phong texture-map";
        phongTexMaterial.shaderProgram = CreateShaderProgram("assets/shaders/phong-tex.vs", "assets/shaders/phong-tex.fs");
        m_Materials.push_back(phongTexMaterial);
    }

    MaterialManager::ShaderProgram MaterialManager::CreateShaderProgram(const std::string& vertexPath,
        const std::string& fragmentPath)
    {
        ShaderProgram shaderProgram;
        std::string vertexCode = ReadShaderFile(vertexPath);
        std::string fragmentCode = ReadShaderFile(fragmentPath);
        if (vertexCode.empty() || fragmentCode.empty()) {
            return {0, "Invalid Shader", vertexPath, fragmentPath};
        }
        GLuint id = Engine::Get().GetRenderSystem().CompileShaders(vertexCode, fragmentCode);
        if (id == 0)
        {
            return {0, "Invalid Shader", vertexPath, fragmentPath};
        }
        shaderProgram.id = id;
        shaderProgram.vertexPath = vertexPath;
        shaderProgram.fragmentPath = fragmentPath;
        shaderProgram.name = vertexPath + "+" + fragmentPath;
        return shaderProgram;
    }

    std::string MaterialManager::ReadShaderFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOGE("Failed to open shader file %s", path.c_str());
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        LOGI("Read shader file %s", path.c_str());
        return buffer.str();
    }

    void MaterialManager::CheckShaderErrors(GLuint shader, const std::string& type)
    {
    }

    void MaterialManager::RenderUI()
    {
        if (m_Materials.empty())
        {
            return;
        }

        const char* materialNames[] = {"Phong (Improved)", "Unlit Texture"};
        ImGui::Combo("Material Type", &m_CurrentMaterialIndex, materialNames, IM_ARRAYSIZE(materialNames));

        Material& currentMat = m_Materials[m_CurrentMaterialIndex];

        // Texture loading
        ImGui::InputText("Texture Path", m_TexturePathBuffer, sizeof(m_TexturePathBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            LoadTexture(m_TexturePathBuffer);
        }

        // Display current texture
        if (currentMat.textureID) {
            ImGui::Text("Current Texture: %s", currentMat.texturePath.c_str());
            ImGui::Image(ImTextureID((void*)(intptr_t)currentMat.textureID), ImVec2(128, 128));
        }

        // Shader parameters
        if (currentMat.name == "Phong (Improved)") {
            ImGui::SliderFloat("Ambient Strength", &currentMat.ambientStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular Strength", &currentMat.specularStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Shininess", &currentMat.shininess, 2.0f, 256.0f);
            ImGui::ColorEdit3("Light Color", &currentMat.lightColor[0]);
        }

        ImGui::ColorEdit3("Object Tint", &currentMat.objectColor[0]);
    }
}
