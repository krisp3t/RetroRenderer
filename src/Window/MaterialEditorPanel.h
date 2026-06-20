#pragma once

#include "EditorContext.h"
#include <filesystem>
#include <functional>

struct SDL_Window;

namespace RetroRenderer {

class Texture;

class MaterialEditorPanel {
  public:
    using TexturePreviewCallback = std::function<void(const Texture&)>;

    void Draw(EditorContext& editorContext, SDL_Window* window, const TexturePreviewCallback& texturePreview);

  private:
    void OpenTexturePicker(EditorContext& editorContext, SDL_Window* window);

  private:
    std::filesystem::path m_lastTextureDirectory_;
};

} // namespace RetroRenderer
