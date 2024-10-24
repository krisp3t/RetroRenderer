#include "Engine.h"
#include "Base/Logger.h"

namespace RetroRenderer
{
    bool Engine::Init()
    {
        if (!m_DisplaySystem.Init())
        {
            return false;
        }
        if (!m_RenderSystem.Init(m_DisplaySystem))
        {
            return false;
        }
        if (!m_InputSystem.Init(m_DisplaySystem.p_Config))
        {
            return false;
        }
        LOGD("p_Config ref count: %d", m_DisplaySystem.p_Config.use_count());
        return true;
    }

    void Engine::Run()
    {
        bool isRunning = true;
        auto start = SDL_GetTicks();
        auto delta = 0;

        LOGI("Entered main loop");
        while (isRunning)
        {
            start = SDL_GetTicks();

            m_InputSystem.HandleInput(isRunning);
            m_RenderSystem.Render();

            delta = SDL_GetTicks() - start;
        }
    }

    void Engine::Destroy()
    {
    }

    /**
     * Handle synchronous event (immediately, without queue).
	 * Used for events that need to be handled immediately, like window resizing and scene reloading.
     */
	void Engine::DispatchImmediate(const Event& event)
	{
        // map EventType enum to string
		// https://stackoverflow.com/questions/147267/easy-way-to-use-variables-of-enum-types-as-string-in-c
		LOGD("Dispatching immediate event (type: %s)", EventTypeToString(event.type));
        Dispatch(event);
	}

    void Engine::Dispatch(const Event& event)
    {
		switch (event.type)
		{
        case EventType::Scene_Load:
        {
            const SceneLoadEvent& e = static_cast<const SceneLoadEvent&>(event);
            m_RenderSystem.OnLoadScene(e);
            break;
        }
        default:
			LOGW("Unknown event type");
		}
    }
}