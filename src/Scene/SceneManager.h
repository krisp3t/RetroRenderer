#pragma once

#include "../Base/InputActions.h"
#include "../Renderer/RenderServices.h"
#include "Camera.h"
#include "Scene.h"
#include <glm/vec2.hpp>
#include <memory>

namespace RetroRenderer {

class SceneManager {
  public:
    SceneManager() = default;
    ~SceneManager();
    void BindDependencies(std::shared_ptr<Config> config, IRenderInvalidationSink& renderInvalidationSink);
    void ResetScene();
    bool LoadScene(const uint8_t* data, const size_t size, bool append = false);
    bool LoadScene(const std::string& path, bool append = false);
    bool ProcessInput(InputActionMask actions, unsigned int deltaTime);
    void Update(unsigned int deltaTime, const glm::ivec2& renderResolution);
    void NewFrame();
    void RenderUI();
    [[nodiscard]] std::shared_ptr<Scene> GetScene() const;
    [[nodiscard]] Camera* GetCamera() const;

  private:
    void RenderUILight(SceneLight& light, int lightIndex);
    void RenderUIModelRecursive(int modelIndex);

    std::shared_ptr<Config> p_Config_ = nullptr;
    IRenderInvalidationSink* p_RenderInvalidationSink_ = nullptr;
    std::shared_ptr<Scene> p_Scene = nullptr;
    std::unique_ptr<Camera> p_Camera = nullptr;
    float m_MoveFactor = 0.02f;
    float m_RotateFactor = 0.10f;
};

} // namespace RetroRenderer
