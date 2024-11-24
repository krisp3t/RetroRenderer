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
private:
    std::unique_ptr<Scene> m_Scene = nullptr;
    Camera m_Camera;
};

}

