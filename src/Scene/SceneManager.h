#pragma once
#include <memory>
#include "Scene.h"
#include "Camera.h"
#include "../Base/InputActions.h"

namespace RetroRenderer
{

    class SceneManager
    {
    public:
        SceneManager() = default;
        ~SceneManager();
        void ResetScene();
        bool LoadScene(const std::string& path);
        bool ProcessInput(InputActionMask actions, unsigned int deltaTime);
        void Update(unsigned int deltaTime);
        void NewFrame();
        [[nodiscard]] std::shared_ptr<Scene> GetScene() const;
        [[nodiscard]] std::shared_ptr<Camera> GetCamera() const;
    private:
        std::shared_ptr<Scene> p_Scene = nullptr;
        std::shared_ptr<Camera> p_Camera = std::make_shared<Camera>(); // TODO: init only when loading scene
        float m_MoveFactor = 0.02f;
        float m_RotateFactor = 0.10f;
    };

}

