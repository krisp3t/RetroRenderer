#include "Engine.h"
#include "Base/Logger.h"

namespace RetroRenderer
{
    bool Engine::Init()
    {
        auto camera = m_SceneManager.GetCamera();
        if (!m_DisplaySystem.Init(p_Config, camera))
        {
            return false;
        }
        if (!m_InputSystem.Init(p_Config))
        {
            return false;
        }
        if (!m_RenderSystem.Init(m_DisplaySystem))
        {
            return false;
        }
        LOGD("p_Config ref count: %d", p_Config.use_count());
        return true;
    }

    void Engine::Run()
    {
        auto start = SDL_GetTicks();
        auto delta = 0;

        LOGI("Entered main loop");
        for (;;)
        {
            start = SDL_GetTicks();

            auto inputActions = m_InputSystem.HandleInput();
            if (inputActions & static_cast<InputActionMask>(InputAction::QUIT))
            {
                break;
            }
            // TODO: handle camera switching
            m_SceneManager.ProcessInput(inputActions, delta);
            m_SceneManager.Update(delta);
            m_DisplaySystem.BeforeFrame(); // SDL, imgui clear screen
            auto scene = m_SceneManager.GetScene();
            auto camera = m_SceneManager.GetCamera();
            if (scene && camera)
            {
                auto queue = m_RenderSystem.BuildRenderQueue(*scene, *camera);
                m_RenderSystem.Render(queue);
                // m_DisplaySystem.DrawFrame(fb);
            }
            m_DisplaySystem.DrawFrame();
            m_DisplaySystem.SwapBuffers();

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
            LOGD("Attempting to load scene from path: %s", e.scenePath.c_str());
            m_SceneManager.LoadScene(e.scenePath);
            m_RenderSystem.OnLoadScene(e); // TODO: send to all subscribers
            break;
        }
        default:
			LOGW("Unknown event type");
		}
    }
}