#include <array>
#include "../Base/Logger.h"
#include "SceneManager.h"
#include "../Base/InputActions.h"

namespace RetroRenderer
{
    SceneManager::~SceneManager()
    {
    }

    bool SceneManager::LoadScene(const std::string& path)
    {
        m_Scene = std::make_unique<Scene>(path);
        return true;
    }

    void SceneManager::ProcessInput(InputActionMask actions)
    {
        if (actions == 0)
        {
            return;
        }
        LOGD("Processing %d input actions in scene manager", actions);
    }

    void SceneManager::Update(unsigned int deltaTime)
    {

    }
}