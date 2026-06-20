#include "MaterialEditorPanel.h"

#include "../Scene/MaterialManager.h"
#include "../Scene/Scene.h"
#include "../native/FileDialog.h"
#include <KrisLogger/Logger.h>
#include <imgui.h>
#include <algorithm>
#include <array>

#ifdef __ANDROID__
#include "../native/android/AndroidBridge.h"
#include <SDL.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace RetroRenderer {
namespace {

struct BuiltInMaterialTemplateOption {
    const char* label;
    const char* assetPath;
};

constexpr std::array<BuiltInMaterialTemplateOption, 5> kBuiltInMaterialTemplates = {{
    {"Phong textured", kMaterialAssetPhongTextured},
    {"Phong vertex color", kMaterialAssetPhongVertexColor},
    {"Phong constant", kMaterialAssetPhongConstant},
    {"Unlit textured", kMaterialAssetUnlitTextured},
    {"Unlit vertex color", kMaterialAssetUnlitVertexColor},
}};

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

std::filesystem::path GetDefaultTextureLocation(const std::filesystem::path& lastTextureDirectory) {
    if (!lastTextureDirectory.empty()) {
        return lastTextureDirectory;
    }
    return std::filesystem::path("assets");
}

MaterialParameterOverride& FindOrCreateParameterOverride(SceneMaterial& material, const MaterialParameterDesc& parameter) {
    auto it = std::find_if(material.parameterOverrides.begin(), material.parameterOverrides.end(), [&](const MaterialParameterOverride& overrideValue) {
        return overrideValue.name == parameter.name;
    });
    if (it != material.parameterOverrides.end()) {
        return *it;
    }

    material.parameterOverrides.push_back(MaterialParameterOverride{
        .name = parameter.name,
        .value = parameter.defaultValue,
    });
    return material.parameterOverrides.back();
}

bool DrawParameterEditor(SceneMaterial& material, const MaterialParameterDesc& parameter) {
    MaterialParameterOverride& overrideValue = FindOrCreateParameterOverride(material, parameter);
    overrideValue.value.type = parameter.type;

    switch (parameter.type) {
    case MaterialDataType::FLOAT1:
        return ImGui::DragFloat(parameter.name.c_str(), &overrideValue.value.data.x, 0.01f);
    case MaterialDataType::VEC2:
        return ImGui::DragFloat2(parameter.name.c_str(), &overrideValue.value.data.x, 0.01f);
    case MaterialDataType::VEC3:
        return ImGui::ColorEdit3(parameter.name.c_str(), &overrideValue.value.data.x);
    case MaterialDataType::VEC4:
        return ImGui::ColorEdit4(parameter.name.c_str(), &overrideValue.value.data.x);
    case MaterialDataType::BOOL1: {
        bool enabled = overrideValue.value.data.x >= 0.5f;
        if (!ImGui::Checkbox(parameter.name.c_str(), &enabled)) {
            return false;
        }
        overrideValue.value.data.x = enabled ? 1.0f : 0.0f;
        return true;
    }
    }
    return false;
}

const char* ResolveMaterialDisplayName(const SceneMaterial& material) {
    if (!material.name.empty()) {
        return material.name.c_str();
    }
    return nullptr;
}

bool DrawMaterialSelector(MaterialManager& materialManager, const std::shared_ptr<Scene>& scene) {
    if (!scene || scene->GetMaterialCount() == 0) {
        return false;
    }

    SceneMaterialHandle selectedHandle = materialManager.GetSelectedSceneMaterialHandle();
    if (selectedHandle == kInvalidSceneMaterialHandle || selectedHandle >= scene->GetMaterialCount()) {
        selectedHandle = 0;
        materialManager.SelectSceneMaterial(selectedHandle);
    }

    bool changed = false;
    const SceneMaterial* selectedMaterial = scene->GetMaterial(selectedHandle);
    const char* previewLabel = selectedMaterial != nullptr ? ResolveMaterialDisplayName(*selectedMaterial) : nullptr;
    std::string fallbackLabel;
    if (previewLabel == nullptr) {
        fallbackLabel = "Material " + std::to_string(selectedHandle);
        previewLabel = fallbackLabel.c_str();
    }

    if (ImGui::BeginCombo("Scene Material", previewLabel)) {
        for (size_t materialIndex = 0; materialIndex < scene->GetMaterialCount(); materialIndex++) {
            const SceneMaterial* material = scene->GetMaterial(static_cast<SceneMaterialHandle>(materialIndex));
            if (material == nullptr) {
                continue;
            }
            const bool isSelected = materialIndex == selectedHandle;
            const char* displayName = ResolveMaterialDisplayName(*material);
            std::string generatedName;
            if (displayName == nullptr) {
                generatedName = "Material " + std::to_string(materialIndex);
                displayName = generatedName.c_str();
            }
            if (ImGui::Selectable(displayName, isSelected)) {
                materialManager.SelectSceneMaterial(static_cast<SceneMaterialHandle>(materialIndex));
                changed = true;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool DrawTemplateSelector(SceneMaterial& material) {
    int selectedIndex = 0;
    const std::string currentTemplate = material.templatePath.generic_string();
    for (size_t optionIndex = 0; optionIndex < kBuiltInMaterialTemplates.size(); optionIndex++) {
        if (currentTemplate == kBuiltInMaterialTemplates[optionIndex].assetPath) {
            selectedIndex = static_cast<int>(optionIndex);
            break;
        }
    }

    const char* previewLabel = kBuiltInMaterialTemplates[static_cast<size_t>(selectedIndex)].label;
    if (!ImGui::BeginCombo("Template", previewLabel)) {
        return false;
    }

    bool changed = false;
    for (size_t optionIndex = 0; optionIndex < kBuiltInMaterialTemplates.size(); optionIndex++) {
        const bool isSelected = static_cast<int>(optionIndex) == selectedIndex;
        if (ImGui::Selectable(kBuiltInMaterialTemplates[optionIndex].label, isSelected)) {
            material.templatePath = kBuiltInMaterialTemplates[optionIndex].assetPath;
            changed = true;
        }
        if (isSelected) {
            ImGui::SetItemDefaultFocus();
        }
    }
    ImGui::EndCombo();
    return changed;
}

} // namespace

void MaterialEditorPanel::Draw(EditorContext& editorContext,
                               SDL_Window* window,
                               const TexturePreviewCallback& texturePreview) {
    ImGui::Begin("Material Editor");

    MaterialManager& materialManager = editorContext.GetMaterialManager();
    const std::shared_ptr<Scene> scene = editorContext.GetScene();
    if (!scene || scene->GetMaterialCount() == 0) {
        ImGui::TextDisabled("Load a scene with mesh materials to edit them.");
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Material");
    DrawMaterialSelector(materialManager, scene);

    SceneMaterial* currentMaterial = materialManager.GetSelectedSceneMaterial();
    if (currentMaterial == nullptr) {
        ImGui::TextDisabled("No material is currently selected.");
        ImGui::End();
        return;
    }

    bool sceneChanged = DrawTemplateSelector(*currentMaterial);
    ImGui::TextDisabled("%s", currentMaterial->templatePath.generic_string().c_str());

    ImGui::SeparatorText("Texture");
    if (ImGui::Button("Load texture")) {
        OpenTexturePicker(editorContext, window);
    }
    ImGui::SameLine();
    if (ImGui::Button("Unload texture")) {
        materialManager.ClearTexture();
    }

    if (const std::shared_ptr<const Texture> previewTexture = materialManager.GetSelectedPreviewTextureShared();
        previewTexture != nullptr && previewTexture->HasCpuPixels()) {
        ImGui::Text("Current Texture: %s (%d x %d)",
                    previewTexture->GetPath().c_str(),
                    previewTexture->GetWidth(),
                    previewTexture->GetHeight());
        if (texturePreview) {
            texturePreview(previewTexture);
        }
    } else {
        ImGui::Text("Current Texture: none");
    }

    ImGui::SeparatorText("Shader Parameters");
    const std::shared_ptr<const CompiledMaterialTemplate> compiledTemplate = materialManager.GetCompiledTemplate(*currentMaterial);
    if (!compiledTemplate) {
        ImGui::TextDisabled("Failed to load the selected material template.");
    } else if (compiledTemplate->parameters.empty()) {
        ImGui::TextDisabled("This material has no editable parameters.");
    } else {
        for (const MaterialParameterDesc& parameter : compiledTemplate->parameters) {
            sceneChanged |= DrawParameterEditor(*currentMaterial, parameter);
        }
    }

    if (sceneChanged) {
        editorContext.GetSceneManager().NotifySceneMutated();
    }

    ImGui::End();
}

void MaterialEditorPanel::OpenTexturePicker(EditorContext& editorContext, SDL_Window* window) {
#ifdef __ANDROID__
    auto* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    auto activity = static_cast<jobject>(SDL_AndroidGetActivity());
    jclass cls = env->GetObjectClass(activity);
    jmethodID mid = env->GetMethodID(cls, "openTexturePicker", "()V");
    env->CallVoidMethod(activity, mid);
#elif defined(__EMSCRIPTEN__)
    (void)editorContext;
    (void)window;
    OpenWebTexturePicker_JS();
#else
    NativeFileDialog::FileDialogRequest request{};
    request.parentWindow = window;
    request.title = "Choose texture";
    request.defaultLocation = GetDefaultTextureLocation(m_lastTextureDirectory_);
    request.filters.push_back(NativeFileDialog::FileDialogFilter{"PNG images", {"*.png"}});
    if (const auto texturePath = NativeFileDialog::ShowOpenFileDialog(request)) {
        LOGD("Selected texture file: %s", texturePath->string().c_str());
        m_lastTextureDirectory_ = texturePath->parent_path();
        editorContext.GetMaterialManager().LoadTexture(texturePath->string());
    }
#endif
}

} // namespace RetroRenderer
