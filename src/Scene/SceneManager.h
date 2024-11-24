#pragma once
#include <memory>
#include "Scene.h"

namespace RetroRenderer
{

class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager();
    bool LoadScene(const std::string& path);
    void ProcessInput(int actions);
    void Update(unsigned int deltaTime);
private:
    std::unique_ptr<Scene> m_Scene = nullptr;
    //std::unique_ptr<Camera> m_Camera = nullptr;
};

}

