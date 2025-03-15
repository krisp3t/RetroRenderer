#pragma once
#include <memory>
#include "Window/DisplaySystem.h"
#include "Renderer/RenderSystem.h"
#include "Window/InputSystem.h"
#include "Scene/SceneManager.h"
#include "Base/Event.h"
#include "Base/Stats.h"

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
	const std::shared_ptr<Stats> GetStats() const;
private:
	Engine() = default;
	~Engine() = default;
    std::shared_ptr<Config> p_config_ = std::make_shared<Config>();
	std::shared_ptr<Stats> p_stats_ = std::make_shared<Stats>();
    DisplaySystem m_DisplaySystem;
    RenderSystem m_RenderSystem;
    InputSystem m_InputSystem;
    SceneManager m_SceneManager;
};

}
