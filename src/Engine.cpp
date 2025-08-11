#include <KrisLogger/Logger.h>
#include "Engine.h"

namespace RetroRenderer
{
    bool Engine::Init()
    {
        LOGD("Starting RetroRenderer in directory: %s", SDL_GetBasePath());
        auto camera = m_SceneManager.GetCamera();
        if (!m_DisplaySystem.Init(camera))
        {
            return false;
        }
        if (!m_InputSystem.Init())
        {
            return false;
        }
        if (!m_RenderSystem.Init(m_DisplaySystem.GetWindow()))
        {
            return false;
        }
        LOGD("p_Config_ ref count: %d", p_config_.use_count());

        // Default scene (optional)
        // m_SceneManager.LoadScene("frog/frog.obj");

        //m_SceneManager.LoadScene("tests-visual/basic-tests/03-3d-cube/model-quad.obj");
        return true;
    }

    void Engine::Run()
    {
        auto start = SDL_GetTicks();
        unsigned int delta = 0;

        LOGD("Entered main loop");
        for (;;)
        {
            start = SDL_GetTicks();

            ProcessEventQueue();

            auto inputActions = m_InputSystem.HandleInput();
            if (inputActions & static_cast<InputActionMask>(InputAction::QUIT))
            {
                break;
            }
            // TODO: handle camera switching

            // TODO: optimisation?
            /*
			if (m_SceneManager.ProcessInput(inputActions, delta))
			{
				m_SceneManager.Update(delta);
			}
            */
            m_SceneManager.ProcessInput(inputActions, delta);
            m_SceneManager.Update(delta);
            m_SceneManager.NewFrame();

            Color clearColor = Color{p_config_->renderer.clearColor};
            auto scene = m_SceneManager.GetScene();
            auto camera = m_SceneManager.GetCamera();
            if (scene && camera)
            {
                m_DisplaySystem.BeforeFrame();
                m_RenderSystem.BeforeFrame(clearColor);
                auto &queue = m_RenderSystem.BuildRenderQueue(*scene, *camera);
                GLuint fbTex = m_RenderSystem.Render(queue, scene->m_Models);
                m_DisplaySystem.DrawFrame(fbTex);
            } else
            {
                p_stats_->Reset();
                m_DisplaySystem.BeforeFrame();
                m_DisplaySystem.DrawFrame();
            }

            m_DisplaySystem.SwapBuffers();

            delta = SDL_GetTicks() - start;
        }
    }

    void Engine::Destroy()
    {
    }

    void Engine::EnqueueEvent(std::unique_ptr<Event> event)
    {
        std::lock_guard<std::mutex> lock(m_EventQueueMutex);
        m_EventQueue.push(std::move(event));
    }

    void Engine::ProcessEventQueue()
    {
        std::queue<std::unique_ptr<Event>> tempQueue;
        {
            // Move all events out of the main queue in one lock
            std::lock_guard<std::mutex> lock(m_EventQueueMutex);
            std::swap(m_EventQueue, tempQueue);
        }
        // Process outside of lock to avoid holding it during event handling
        while (!tempQueue.empty())
        {
            auto &event = *tempQueue.front();
            Dispatch(event);
            tempQueue.pop();
        }
    }

    /**
     * Handle synchronous event (immediately, without queue).
	 * Used for events that need to be handled immediately, like window resizing and scene reloading.
     */
    void Engine::DispatchImmediate(const Event &event)
    {
        // map EventType enum to string
        // https://stackoverflow.com/questions/147267/easy-way-to-use-variables-of-enum-types-as-string-in-c
        LOGD("Dispatching immediate event (type: %s)", EventTypeToString(event.type));
        Dispatch(event);
    }

    void Engine::Dispatch(const Event &event)
    {
        switch (event.type)
        {
            case EventType::Scene_Load:
            {
                const SceneLoadEvent &e = static_cast<const SceneLoadEvent &>(event);
                if (!e.loadFromMemory)
                {
                    LOGD("Attempting to load scene from path: %s", e.scenePath.c_str());
                    m_SceneManager.LoadScene(e.scenePath);
                }
                else
                {
                    m_SceneManager.LoadScene(e.sceneDataBuffer.data(), e.sceneDataSize);
                }
                m_RenderSystem.OnLoadScene(e); // TODO: send to all subscribers
                break;
            }
            case EventType::Scene_Reset:
            {
                m_SceneManager.ResetScene();
                break;
            }
            case EventType::Output_Image_Resize:
            {
                const OutputImageResizeEvent &e = static_cast<const OutputImageResizeEvent &>(event);
                m_RenderSystem.Resize(e.resolution);
                break;
            }
            default:
                LOGW("Unknown event type");
        }
    }

    const std::shared_ptr<Config> Engine::GetConfig() const
    {
        return p_config_;
    }

	const std::shared_ptr<Stats> Engine::GetStats() const
	{
		return p_stats_;
	}
}
