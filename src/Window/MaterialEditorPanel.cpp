#include "MaterialEditorPanel.h"

#include "../Scene/MaterialManager.h"
#include "../native/FileDialog.h"
#include <KrisLogger/Logger.h>
#include <imgui.h>

#ifdef __ANDROID__
#include "../native/android/AndroidBridge.h"
#include <SDL.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace RetroRenderer {
namespace {

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

} // namespace

void MaterialEditorPanel::Draw(EditorContext& editorContext,
                               SDL_Window* window,
                               const TexturePreviewCallback& texturePreview) {
    ImGui::Begin("Material Editor");

    MaterialManager& materialManager = editorContext.GetMaterialManager();
    ImGui::SeparatorText("Material");
    constexpr const char* kMaterialNames[] = {"Phong (texture color)", "Phong (vertex color)"};
    int currentMaterialIndex = materialManager.GetCurrentMaterialIndex();
    if (ImGui::Combo("Material Type", &currentMaterialIndex, kMaterialNames, IM_ARRAYSIZE(kMaterialNames))) {
        materialManager.SetCurrentMaterialIndex(currentMaterialIndex);
    }

    MaterialManager::Material& currentMaterial = materialManager.GetCurrentMaterial();

    ImGui::SeparatorText("Texture");
    if (ImGui::Button("Load texture")) {
        OpenTexturePicker(editorContext, window);
    }
    ImGui::SameLine();
    if (ImGui::Button("Unload texture")) {
        materialManager.ClearTexture();
    }

    if (currentMaterial.texture.has_value() && currentMaterial.texture->HasCpuPixels()) {
        ImGui::Text("Current Texture: %s (%d x %d)",
                    currentMaterial.texture->GetPath().c_str(),
                    currentMaterial.texture->GetWidth(),
                    currentMaterial.texture->GetHeight());
        if (texturePreview) {
            texturePreview(*currentMaterial.texture);
        }
    } else {
        ImGui::Text("Current Texture: none");
    }

    ImGui::SeparatorText("Shader Parameters");
    if (currentMaterial.phongParams.has_value()) {
        ImGui::SliderFloat("Ambient Strength", &currentMaterial.phongParams->ambientStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular Strength", &currentMaterial.phongParams->specularStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Shininess", &currentMaterial.phongParams->shininess, 2.0f, 256.0f);
        ImGui::ColorEdit3("Light Color", &currentMaterial.lightColor[0]);
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
