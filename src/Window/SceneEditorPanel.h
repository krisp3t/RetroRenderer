#pragma once

#include "EditorContext.h"

namespace RetroRenderer {

class Scene;
struct SceneLight;

class SceneEditorPanel {
  public:
    void Draw(EditorContext& editorContext);

  private:
    void DrawLight(SceneLight& light, int lightIndex, EditorContext& editorContext);
    void DrawModelRecursive(Scene& scene, int modelIndex, EditorContext& editorContext);
};

} // namespace RetroRenderer
