#include "MaterialManager.h"

#include "../Renderer/RenderServices.h"
#include <KrisLogger/Logger.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>

#ifdef __ANDROID__
#include "../native/android/AndroidBridge.h"
#endif

namespace RetroRenderer {

void MaterialManager::BindRenderServices(IRenderInvalidationSink& renderInvalidationSink) {
    p_RenderInvalidationSink_ = &renderInvalidationSink;
}

bool MaterialManager::Init() {
    if (p_RenderInvalidationSink_ == nullptr) {
        LOGE("MaterialManager render services were not bound before initialization");
        return false;
    }
    LoadDefaultShaders();
    return true;
}

void MaterialManager::LoadTexture(const std::string& path) {
    Texture texture;
    if (!texture.LoadFromFile(path.c_str())) {
        LOGE("Failed to load texture %s", path.c_str());
        return;
    }
    GetCurrentMaterial().texture = std::move(texture);
    p_RenderInvalidationSink_->OnTextureMutated();
}

void MaterialManager::LoadTexture(const uint8_t* data, const size_t size) {
    Texture texture;
    if (!texture.LoadFromMemory(data, size)) {
        LOGE("Failed to load texture from memory");
        return;
    }
    GetCurrentMaterial().texture = std::move(texture);
    p_RenderInvalidationSink_->OnTextureMutated();
}

void MaterialManager::ClearTexture() {
    if (m_Materials.empty()) {
        return;
    }
    GetCurrentMaterial().texture.reset();
    p_RenderInvalidationSink_->OnTextureMutated();
}

void MaterialManager::ResetBuiltInMaterials() {
    bool textureChanged = false;
    for (Material& material : m_Materials) {
        textureChanged |= material.texture.has_value();
        material.texture.reset();
        material.phongParams = PhongParams{};
        material.lightColor = glm::vec3(1.0f);
    }

    if (textureChanged && p_RenderInvalidationSink_ != nullptr) {
        p_RenderInvalidationSink_->OnTextureMutated();
    }
}

void MaterialManager::LoadDefaultShaders() {
    // TODO: extract
    Material phongTexMaterial;
    phongTexMaterial.name = "phong-tex";
    phongTexMaterial.phongParams = PhongParams{};
    Material phongVcMaterial;
    phongVcMaterial.name = "phong-vc";
    phongVcMaterial.phongParams = PhongParams{};
    auto phongTexVs = "shaders/phong-tex.vs";
    auto phongTexFs = "shaders/phong-tex.fs";
    auto phongVcVs = "shaders/phong-vc.vs";
    auto phongVcFs = "shaders/phong-vc.fs";

    phongTexMaterial.shaderProgram = CreateShaderProgram(phongTexVs, phongTexFs);
    m_Materials.emplace_back(std::move(phongTexMaterial));
    phongVcMaterial.shaderProgram = CreateShaderProgram(phongVcVs, phongVcFs);
    m_Materials.emplace_back(std::move(phongVcMaterial));
}

void MaterialManager::SetCurrentMaterialIndex(int materialIndex) {
    if (m_Materials.empty()) {
        m_CurrentMaterialIndex = 0;
        return;
    }
    m_CurrentMaterialIndex = std::clamp(materialIndex, 0, static_cast<int>(m_Materials.size()) - 1);
}

ShaderProgram MaterialManager::CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string rootPath = "assets/";
#ifdef __ANDROID__
    rootPath = g_assetsPath;
#endif
    std::string vertexCode = ReadShaderFile(rootPath + vertexPath);
    std::string fragmentCode = ReadShaderFile(rootPath + fragmentPath);
    if (vertexCode.empty() || fragmentCode.empty()) {
        return {ShaderHandle{}, nullptr, "Invalid Shader", vertexPath, fragmentPath};
    }

    auto source = std::make_shared<RenderShaderSnapshot>();
    source->vertexCode = std::move(vertexCode);
    source->fragmentCode = std::move(fragmentCode);
    return {ShaderHandle{}, std::move(source), vertexPath + "+" + fragmentPath, vertexPath, fragmentPath};
}

ShaderProgram MaterialManager::CreateShaderProgram(IShaderCompiler& shaderCompiler,
                                                   const std::string& vertexPath,
                                                   const std::string& fragmentPath) {
    ShaderProgram shaderProgram;
    std::string rootPath = "assets/";
#ifdef __ANDROID__
    rootPath = g_assetsPath;
#endif
    std::string vertexCode = ReadShaderFile(rootPath + vertexPath);
    std::string fragmentCode = ReadShaderFile(rootPath + fragmentPath);
    if (vertexCode.empty() || fragmentCode.empty()) {
        return {ShaderHandle{}, nullptr, "Invalid Shader", vertexPath, fragmentPath};
    }
    ShaderHandle handle = shaderCompiler.CompileShaders(vertexCode, fragmentCode);
    if (!handle.IsValid()) {
        return {ShaderHandle{}, nullptr, "Invalid Shader", vertexPath, fragmentPath};
    }
    shaderProgram.handle = handle;
    shaderProgram.vertexPath = vertexPath;
    shaderProgram.fragmentPath = fragmentPath;
    shaderProgram.name = vertexPath + "+" + fragmentPath;
    LOGI("Created shader program %s with handle %zu", shaderProgram.name.c_str(), static_cast<size_t>(shaderProgram.handle.value));
    return shaderProgram;
}

std::string MaterialManager::ReadShaderFile(const std::string& path) {
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

} // namespace RetroRenderer
