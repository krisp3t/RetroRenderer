#pragma once
#include <memory>
#include "Window/DisplaySystem.h"
#include "Renderer/RenderSystem.h"
#include "Window/InputSystem.h"
#include "Scene/SceneManager.h"
#include "Base/Event.h"

namespace RetroRenderer
{

class Engine
{
public:
	static Engine& Get()
	{
		static Engine instance;
		return instance;
	}

    bool Init();
    void Run();
    void Destroy();
    void DispatchImmediate(const Event& event);
	void Dispatch(const Event& event);
	const std::shared_ptr<Config> GetConfig() const;
private:
	Engine() = default;
	~Engine() = default;
    std::shared_ptr<Config> p_Config = std::make_shared<Config>();
    DisplaySystem m_DisplaySystem;
    RenderSystem m_RenderSystem;
    InputSystem m_InputSystem;
    SceneManager m_SceneManager;
};

}
