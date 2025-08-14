#include "MaterialManager.h"

#include <fstream>
#include <imgui.h>
#include <sstream>
#include "../../lib/ImGuiFileDialog/ImGuiFileDialog.h"
#include "../Engine.h"
#include "KrisLogger/Logger.h"

#ifdef __ANDROID__
#include "../native/AndroidBridge.h"
#endif

namespace RetroRenderer
{
    bool MaterialManager::Init()
    {
        LoadDefaultShaders();
        return true;
    }

    void MaterialManager::LoadTexture(const std::string& path)
    {
        Texture texture;
        if (!texture.LoadFromFile(path.c_str())) {
            LOGE("Failed to load texture %s", path.c_str());
            return;
        }
        GetCurrentMaterial().texture = std::move(texture);
        // TODO: resource leak
    }

    void MaterialManager::LoadTexture(const uint8_t* data, const size_t size)
    {
        Texture texture;
        if (!texture.LoadFromMemory(data, size)) {
            LOGE("Failed to load texture from memory");
            return;
        }
        GetCurrentMaterial().texture = std::move(texture);
    }

    void MaterialManager::LoadDefaultShaders()
    {
        // TODO: extract
        Material phongTexMaterial;
        phongTexMaterial.name = "phong-tex";
        phongTexMaterial.phongParams = PhongParams{};
        phongTexMaterial.texture = Texture{};
        Material phongVcMaterial;
        phongVcMaterial.name = "phong-vc";
        phongVcMaterial.phongParams = PhongParams{};
#ifdef __ANDROID__
        auto phongTexVs = g_assetsPath + "/shaders/phong-tex.vs";
        auto phongTexFs = g_assetsPath + "/shaders/phong-tex.fs";
        auto phongVcVs = g_assetsPath + "/shaders/phong-vc.vs";
        auto phongVcFs = g_assetsPath + "/shaders/phong-vc.fs";
#else
        auto phongTexVs = "assets/shaders/phong-tex.vs";
        auto phongTexFs = "assets/shaders/phong-tex.fs";
        auto phongVcVs = "assets/shaders/phong-vc.vs";
        auto phongVcFs = "assets/shaders/phong-vc.fs";
#endif
        
        phongTexMaterial.shaderProgram = CreateShaderProgram(phongTexVs, phongTexFs);
        m_Materials.emplace_back(std::move(phongTexMaterial));
        phongVcMaterial.shaderProgram = CreateShaderProgram(phongVcVs, phongVcFs);
        m_Materials.emplace_back(std::move(phongVcMaterial));
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
        LOGI("Created shader program %s with handle %d", shaderProgram.name.c_str(), shaderProgram.id);
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
        ImGui::SeparatorText("Material");
        const char* materialNames[] = {"Phong (texture color)", "Phong (vertex color)"};
        ImGui::Combo("Material Type", &m_CurrentMaterialIndex, materialNames, IM_ARRAYSIZE(materialNames));

        Material& currentMat = GetCurrentMaterial();

        // Texture loading
        ImGui::SeparatorText("Texture");
        if (currentMat.texture.has_value())
        {
            if (ImGui::Button("Load texture")) {
                // TODO: extract platform specific
#ifdef __ANDROID__
                auto* env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
                auto activity = static_cast<jobject>(SDL_AndroidGetActivity());
                jclass cls = env->GetObjectClass(activity);
                jmethodID mid = env->GetMethodID(cls, "openTexturePicker", "()V");
                env->CallVoidMethod(activity, mid);
#else
                IGFD::FileDialogConfig sceneDialogConfig;
                ImGuiFileDialog::Instance()->Close();
                ImGuiFileDialog::Instance()->OpenDialog("OpenTextureFile", "Choose texture", k_supportedTextures,
                                            sceneDialogConfig);
#endif
            }
            ImGui::SameLine();
            if (ImGui::Button("Unload texture"))
            {
                currentMat.texture = Texture();
            }

            // Display current texture
            if (currentMat.texture->GetID()) {
                ImGui::Text("Current Texture: handle %d (%s)", currentMat.texture->GetID(), currentMat.texture->GetPath().c_str());
                ImGui::Image(ImTextureID((void*)(intptr_t)currentMat.texture->GetID()), ImVec2(128, 128));
            } else
            {
                ImGui::Text("Current Texture: none");
            }
        }
        // Shader parameters
        ImGui::SeparatorText("Shader Parameters");
        if (currentMat.phongParams.has_value()) {
            ImGui::SliderFloat("Ambient Strength", &currentMat.phongParams->ambientStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular Strength", &currentMat.phongParams->specularStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Shininess", &currentMat.phongParams->shininess, 2.0f, 256.0f);
            ImGui::ColorEdit3("Light Color", &currentMat.lightColor[0]);
        }
    }
}
