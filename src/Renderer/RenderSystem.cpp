#include <cassert>
#include <iostream>
#include "RenderSystem.h"
#include "../Base/Logger.h"
#include "../Engine.h"

namespace RetroRenderer
{
    void RenderSystem::CreateFramebufferTexture(GLuint &texId, int width, int height)
    {
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                width,
                height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                nullptr
        );
        // TODO: make filtering configurable
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    bool RenderSystem::Init(SDL_Window *window, std::shared_ptr<Stats> stats)
    {
        p_Stats = stats;

        auto &p_Config = Engine::Get().GetConfig();
        p_SWRenderer = std::make_unique<SWRenderer>();
        p_GLRenderer = std::make_unique<GLRenderer>();
        p_activeRenderer = p_SWRenderer.get();
        auto &fbResolution = p_Config->renderer.resolution;
        assert(fbResolution.x > 0 && fbResolution.y > 0 && "Tried to initialize renderers with invalid resolution");

        CreateFramebufferTexture(m_SWFramebufferTexture, fbResolution.x, fbResolution.y);
        CreateFramebufferTexture(m_GLFramebufferTexture, fbResolution.x, fbResolution.y);

        if (!p_SWRenderer->Init(m_SWFramebufferTexture, fbResolution.x, fbResolution.y))
        {
            LOGE("Failed to initialize SWRenderer");
            return false;
        }
        if (!p_GLRenderer->Init(m_GLFramebufferTexture, fbResolution.x, fbResolution.y))
        {
            LOGE("Failed to initialize GLRenderer");
            return false;
        }

        return true;
    }

    void RenderSystem::BeforeFrame(const Color &clearColor)
    {
        auto &p_Config = Engine::Get().GetConfig();
        switch (p_Config->renderer.selectedRenderer)
        {
            case Config::RendererType::SOFTWARE:
                p_activeRenderer = p_SWRenderer.get();
                break;
            case Config::RendererType::GL:
                p_activeRenderer = p_GLRenderer.get();
                break;
            default:
                LOGE("Renderer type %d not implemented!", p_Config->renderer.selectedRenderer);
                return;
        }
        assert(p_activeRenderer != nullptr && "Active renderer is null");
        p_activeRenderer->BeforeFrame(clearColor);
    }

    std::queue<Model *> &RenderSystem::BuildRenderQueue(Scene &scene, const Camera &camera)
    {
        auto &activeRenderer = p_SWRenderer; // TODO: get from config
        activeRenderer->SetActiveCamera(camera);
        return scene.GetVisibleModels(); // TODO: split into meshes?
    }

    GLuint RenderSystem::Render(std::queue<Model *> &renderQueue)
    {
        assert(p_Stats != nullptr && "Stats not initialized!");
        p_Stats->Reset();

        auto p_Config = Engine::Get().GetConfig();
        auto selectedRenderer = p_Config->renderer.selectedRenderer;
        IRenderer *activeRenderer = nullptr;
        switch (selectedRenderer)
        {
            case Config::RendererType::SOFTWARE:
                activeRenderer = p_SWRenderer.get();
                break;
            case Config::RendererType::GL:
                activeRenderer = p_GLRenderer.get();
                break;
            default:
                LOGE("Renderer type %d not implemented!", selectedRenderer);
                return 0;
        }
        assert(activeRenderer != nullptr && "Active renderer is null");

        //LOGD("Render queue size: %d", renderQueue.size());
        while (!renderQueue.empty())
        {
            const Model *model = renderQueue.front();
            assert(model != nullptr && "Model is null");
            activeRenderer->DrawTriangularMesh(model);
            renderQueue.pop();
        }
        return activeRenderer->EndFrame();
    }

    void RenderSystem::Resize(const glm::ivec2 &resolution)
    {
        auto &p_Config = Engine::Get().GetConfig();
        p_Config->renderer.resolution = resolution;
        CreateFramebufferTexture(m_SWFramebufferTexture, resolution.x, resolution.y);
        CreateFramebufferTexture(m_GLFramebufferTexture, resolution.x, resolution.y);
        LOGI("Resizing output image to %d x %d", resolution.x, resolution.y);
        p_SWRenderer->Resize(resolution.x, resolution.y);
        p_GLRenderer->Resize(resolution.x, resolution.y);
    }

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnLoadScene(const SceneLoadEvent &e)
    {

    }

}