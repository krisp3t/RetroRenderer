#pragma once

#include "Window/DisplaySystem.h"
#include "Renderer/RenderSystem.h"
#include "Window/InputSystem.h"
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
private:
	Engine() = default;
	~Engine() = default;
    DisplaySystem m_DisplaySystem;
    RenderSystem m_RenderSystem;
    InputSystem m_InputSystem;
};

}
