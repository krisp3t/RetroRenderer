#include "MaterialManager.h"
#include <imgui.h>

namespace RetroRenderer
{
    bool MaterialManager::Init()
    {
        return true;
    }

    void MaterialManager::LoadTexture(const std::string& path)
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
