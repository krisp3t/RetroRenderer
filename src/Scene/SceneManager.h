#pragma once
#include <memory>
#include "Scene.h"
#include "Camera.h"

namespace RetroRenderer
{

class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager();
    bool LoadScene(const std::string& path);
    void ProcessInput(int actions, unsigned int deltaTime);
    void Update(unsigned int deltaTime);
    [[nodiscard]] std::shared_ptr<Camera> GetCamera() const;
private:
    std::shared_ptr<Scene> p_Scene = nullptr;
    std::shared_ptr<Camera> p_Camera = std::make_shared<Camera>(); // TODO: init only when loading scene
    float m_MoveFactor = 0.1f;
};

}

