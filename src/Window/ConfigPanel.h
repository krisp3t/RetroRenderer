#pragma once
#include "../Base/ExampleSceneCatalog.h"
#include "../Base/Config.h"
#include "../Base/Stats.h"
#include "../Renderer/FrameSubmission.h"
#include "../Renderer/RenderOutput.h"
#include "EditorContext.h"
#include "MaterialEditorPanel.h"
#include "SceneEditorPanel.h"
#include <SDL.h>
#include <filesystem>
#include <imgui.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace RetroRenderer {
class Texture;

struct VirtualStickState {
    bool active = false;
    ImVec2 origin;              // where finger first touched
    ImVec2 delta;               // normalized [-1, 1]
    SDL_FingerID fingerId = -1; // not needed for mouse, useful for multi-touch
};
class ConfigPanel {
  public:
    ConfigPanel() = default;
    ~ConfigPanel();
    bool Init(SDL_Window* window, SDL_GLContext glContext, std::shared_ptr<Config> config, std::shared_ptr<Stats> stats);
    void BindEditorContext(EditorContext editorContext);
    void Destroy();
    void BeforeFrame();
    void DisplayRenderedImage();
    void DisplayRenderedImage(bool outputAvailable, RenderOutputOrigin origin);
    void OnDraw();
    [[nodiscard]] const UiFontAtlas& GetFontAtlas() const;
    [[nodiscard]] UiRenderPacket TakeUiRenderPacket();
    [[nodiscard]] std::vector<UiTextureSnapshot> TakeUiTextureSnapshots();

  private:
    SDL_Window* p_Window_ = nullptr;
    std::shared_ptr<Config> p_config_ = nullptr;
    std::shared_ptr<Stats> p_stats_ = nullptr;
    std::vector<uint8_t> m_fontData_ = {}; // keep font bytes alive for the lifetime of imgui
    UiFontAtlas m_fontAtlas_;
    UiRenderPacket m_uiRenderPacket_;
    std::vector<UiTextureSnapshot> m_uiTextureSnapshots_;
    std::shared_ptr<const Texture> m_previewTextureSnapshot_;
    const Texture* m_previewTextureSource_ = nullptr;
    uint64_t m_previewTextureRevision_ = 0;
    bool m_isDragging_ = false;
    std::optional<EditorContext> m_editorContext_;
    SceneEditorPanel m_sceneEditorPanel_;
    MaterialEditorPanel m_materialEditorPanel_;
    ExampleSceneCatalog m_exampleSceneCatalog_;
    std::filesystem::path m_lastSceneDirectory_;
    std::optional<size_t> m_selectedExampleDirectoryIndex_;
    std::optional<size_t> m_selectedExampleSceneIndex_;
    std::string m_examplesStatusMessage_;
    bool m_showExamplesWindow_ = false;
    bool m_examplesAutoOpened_ = false;
    bool m_hasManagedSceneBaseline_ = false;

    void StyleColorsEnemymouse();
    void DisplayGUI();
    void DisplayMetricsOverlay();
    void DisplayCameraSettings();
    void DisplayCullSettings();
    void DisplayRendererSettings();
    void DisplayRasterizerSettings();
    void DisplayPostFxSettings();
    void DisplayEnvironmentSettings();
    void DisplayMainMenu();
    void DisplaySceneGraph();
    void DisplayMaterialWindow();
    void DisplayTexturePreview(const Texture& texture);
    void DisplayConfigWindow();
    void DisplayControlsOverlay();
    void DisplayExamplesWindow();
    void DisplayWindowSettings();
    void DisplayJoysticks();
    void ApplyRendererPreset(Config::RenderPreset preset);
    void MarkRendererPresetCustom();
    void DrawExampleDirectoryNode(size_t directoryIndex);
    void ApplyManagedSceneBaselineForPath(const std::filesystem::path& scenePath);
    void LoadSelectedExampleScene();
    void LoadSceneFromPath(const std::filesystem::path& scenePath);
    void OpenExamplesWindow();
    void OpenWebFilePicker();
    void OpenAndroidFilePicker();
    void RefreshExamplesCatalog();
    void ResetManagedSceneBaseline();
    void SelectExampleDirectory(size_t directoryIndex);
    void DispatchImmediate(const Event& event) const;
    [[nodiscard]] Camera* GetCamera() const;
    [[nodiscard]] std::shared_ptr<Scene> GetScene() const;
    [[nodiscard]] bool HasScene() const;

    static constexpr const char* kSupportedModels = ".obj";
    VirtualStickState moveStickState;
    VirtualStickState rotateStickState;
};

} // namespace RetroRenderer
