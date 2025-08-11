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
        bool LoadScene(const uint8_t* data, const size_t size);
        bool LoadScene(const std::string &path);
        bool ProcessInput(InputActionMask actions, unsigned int deltaTime);
        void Update(unsigned int deltaTime);
        void NewFrame();
        [[nodiscard]] std::shared_ptr<Scene> GetScene() const;
        [[nodiscard]] Camera* GetCamera() const;
    private:
        std::shared_ptr<Scene> p_Scene = nullptr;
        std::unique_ptr<Camera> p_Camera = nullptr;
        float m_MoveFactor = 0.02f;
        float m_RotateFactor = 0.10f;
    };

}

