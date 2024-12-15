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
        p_ActiveRenderer = p_SWRenderer.get();
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
                p_ActiveRenderer = p_SWRenderer.get();
                break;
            case Config::RendererType::GL:
                p_ActiveRenderer = p_GLRenderer.get();
                break;
            default:
                LOGE("Renderer type %d not implemented!", p_Config->renderer.selectedRenderer);
                return;
        }
        assert(p_ActiveRenderer != nullptr && "Active renderer is null");
        glViewport(0, 0, p_Config->renderer.resolution.x, p_Config->renderer.resolution.y);
        p_ActiveRenderer->BeforeFrame(clearColor);
    }

    std::vector<int> &RenderSystem::BuildRenderQueue(Scene &scene, const Camera &camera)
    {
        p_ActiveRenderer->SetActiveCamera(camera);
        return scene.GetVisibleModels(); // TODO: split into meshes?
    }

    GLuint RenderSystem::Render(std::vector<int> &renderQueue, std::vector<Model> &models)
    {
        assert(p_Stats != nullptr && "Stats not initialized!");
        p_Stats->Reset();

        //LOGD("Render queue size: %d", renderQueue.size());
        for (int modelIx: renderQueue)
        {
            const Model *model = &models[modelIx];
            assert(model != nullptr && "Model is null");
            if (model->GetMeshes().size() > 0)
            {
                p_ActiveRenderer->DrawTriangularMesh(model);
            }
        }
        return p_ActiveRenderer->EndFrame();
    }

    void RenderSystem::Resize(const glm::ivec2 &resolution)
    {
        auto &p_Config = Engine::Get().GetConfig();
        p_Config->renderer.resolution = resolution;
        CreateFramebufferTexture(m_SWFramebufferTexture, resolution.x, resolution.y);
        CreateFramebufferTexture(m_GLFramebufferTexture, resolution.x, resolution.y);
        LOGI("Resizing output image to %d x %d", resolution.x, resolution.y);
        p_SWRenderer->Resize(m_SWFramebufferTexture, resolution.x, resolution.y);
        p_GLRenderer->Resize(m_GLFramebufferTexture, resolution.x, resolution.y);
    }

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnLoadScene(const SceneLoadEvent &e)
    {

    }

}