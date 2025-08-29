#pragma once
#include <memory>
#include <mutex>

#include "Window/DisplaySystem.h"
#include "Renderer/RenderSystem.h"
#include "Window/InputSystem.h"
#include "Scene/SceneManager.h"
#include "Base/Event.h"
#include "Base/Stats.h"
#include "Scene/MaterialManager.h"

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

    // Events
    void DispatchImmediate(const Event& event);
	void Dispatch(const Event& event);
    void EnqueueEvent(std::unique_ptr<Event> event);

	[[nodiscard]] const std::shared_ptr<Config> GetConfig() const;
	[[nodiscard]] const std::shared_ptr<Stats> GetStats() const;
	[[nodiscard]] Camera* GetCamera() const;
	[[nodiscard]] MaterialManager& GetMaterialManager() const;
	[[nodiscard]] RenderSystem& GetRenderSystem() const;
	[[nodiscard]] SceneManager& GetSceneManager() const;

private:
	Engine() = default;
	~Engine() = default;
    void ProcessEventQueue();

private:

    std::shared_ptr<Config> p_config_ = std::make_shared<Config>();
	std::shared_ptr<Stats> p_stats_ = std::make_shared<Stats>();
    std::queue<std::unique_ptr<Event>> m_EventQueue;
    std::mutex m_EventQueueMutex;

    DisplaySystem m_DisplaySystem;
    std::unique_ptr<RenderSystem> p_RenderSystem;
    InputSystem m_InputSystem;
    std::unique_ptr<SceneManager> p_SceneManager;
	std::unique_ptr<MaterialManager> p_MaterialManager;
};

}
