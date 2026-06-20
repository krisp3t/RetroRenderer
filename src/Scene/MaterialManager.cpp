#include "MaterialManager.h"

#include "../Renderer/RenderServices.h"
#include "../native/FileDialog.h"
#include "KrisLogger/Logger.h"
#include <SDL.h>
#include <cstdint>
#include <fstream>
#include <imgui.h>
#include <sstream>

#ifdef __ANDROID__
#include "../native/AndroidBridge.h"
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace RetroRenderer {
#ifdef __EMSCRIPTEN__
EM_JS(void, OpenWebTexturePicker_JS, (), {
    let input = document.createElement('input');
    input.type = 'file';
    input.accept = '.png';
    input.style.display = 'none';

    input.onchange = e => {
        let file = e.target.files[0];
        if (!file)
            return;

        let reader = new FileReader();
        reader.onload = function(event) {
            let data = event.target.result;
            let size = data.byteLength;
            let ptr = Module._malloc(size);
            if (!ptr) {
                console.error("Failed to allocate memory");
                return;
            }

            let heapBytes = new Uint8Array(Module.HEAPU8.buffer, ptr, size);
            heapBytes.set(new Uint8Array(data));
            Module.ccall('OnWebTextureSelected', null, [ 'number', 'number' ], [ ptr, size ]);
        };
        reader.readAsArrayBuffer(file);
    };

    document.body.appendChild(input);
    input.click();
    document.body.removeChild(input);
});
#endif

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

void MaterialManager::LoadDefaultShaders() {
    // TODO: extract
    Material phongTexMaterial;
    phongTexMaterial.name = "phong-tex";
    phongTexMaterial.phongParams = PhongParams{};
    phongTexMaterial.texture = Texture{};
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

void MaterialManager::RenderUI(const TexturePreviewCallback& texturePreview) {
    if (m_Materials.empty()) {
        return;
    }
    ImGui::SeparatorText("Material");
    constexpr const char* materialNames[] = {"Phong (texture color)", "Phong (vertex color)"};
    ImGui::Combo("Material Type", &m_CurrentMaterialIndex, materialNames, IM_ARRAYSIZE(materialNames));

    Material& currentMat = GetCurrentMaterial();

    // Texture loading
    ImGui::SeparatorText("Texture");
    if (currentMat.texture.has_value()) {
        if (ImGui::Button("Load texture")) {
            // TODO: extract platform specific
#ifdef __ANDROID__
            auto* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
            auto activity = static_cast<jobject>(SDL_AndroidGetActivity());
            jclass cls = env->GetObjectClass(activity);
            jmethodID mid = env->GetMethodID(cls, "openTexturePicker", "()V");
            env->CallVoidMethod(activity, mid);
#elif defined(__EMSCRIPTEN__)
            OpenWebTexturePicker_JS();
#else
            NativeFileDialog::FileDialogRequest request{};
            request.title = "Choose texture";
            request.defaultLocation = m_lastTextureDirectory_.empty() ? std::filesystem::path("assets")
                                                                      : m_lastTextureDirectory_;
            request.filters.push_back(NativeFileDialog::FileDialogFilter{"PNG images", {"*.png"}});
            if (const auto texturePath = NativeFileDialog::ShowOpenFileDialog(request)) {
                LOGD("Selected texture file: %s", texturePath->string().c_str());
                m_lastTextureDirectory_ = texturePath->parent_path();
                LoadTexture(texturePath->string());
            }
#endif
        }
        ImGui::SameLine();
        if (ImGui::Button("Unload texture")) {
            currentMat.texture = Texture();
            p_RenderInvalidationSink_->OnTextureMutated();
        }

        if (currentMat.texture->HasCpuPixels()) {
            ImGui::Text("Current Texture: %s (%d x %d)",
                        currentMat.texture->GetPath().c_str(),
                        currentMat.texture->GetWidth(),
                        currentMat.texture->GetHeight());
            if (texturePreview) {
                texturePreview(*currentMat.texture);
            }
        } else {
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
} // namespace RetroRenderer
