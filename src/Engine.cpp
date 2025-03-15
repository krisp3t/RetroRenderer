#include <KrisLogger/Logger.h>
#include "Engine.h"

namespace RetroRenderer
{
    bool Engine::Init()
    {
        LOGD("Starting RetroRenderer in directory: %s", SDL_GetBasePath());
        auto camera = m_SceneManager.GetCamera();
        if (!m_DisplaySystem.Init(p_Config, camera, p_Stats))
        {
            return false;
        }
        if (!m_InputSystem.Init(p_Config))
        {
            return false;
        }
        if (!m_RenderSystem.Init(m_DisplaySystem.GetWindow(), p_Stats))
        {
            return false;
        }
        LOGD("p_Config_ ref count: %d", p_Config.use_count());

        // Default scene (optional)
        m_SceneManager.LoadScene("tests-visual/basic-tests/01-2d-triangle/model.obj");
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

            Color clearColor = Color{p_Config->renderer.clearColor};
            auto scene = m_SceneManager.GetScene();
            auto camera = m_SceneManager.GetCamera();
            if (scene && camera)
            {
                m_RenderSystem.BeforeFrame(clearColor);
                auto &queue = m_RenderSystem.BuildRenderQueue(*scene, *camera);
                GLuint fbTex = m_RenderSystem.Render(queue, scene->m_Models);
                m_DisplaySystem.BeforeFrame();
                m_DisplaySystem.DrawFrame(fbTex);
            } else
            {
                p_Stats->Reset();
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
                LOGD("Attempting to load scene from path: %s", e.scenePath.c_str());
                m_SceneManager.LoadScene(e.scenePath);
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
        return p_Config;
    }
}
