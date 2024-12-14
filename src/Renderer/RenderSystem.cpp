#include <cassert>
#include <iostream>
#include <random>
#include "RenderSystem.h"
#include "../Base/Logger.h"
#include "../Engine.h"

namespace RetroRenderer
{
	void RenderSystem::CreateFramebufferTexture(GLuint& texId, int width, int height)
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

    bool RenderSystem::Init(DisplaySystem& displaySystem, std::shared_ptr<Stats> stats)
    {
		auto& p_Config = Engine::Get().GetConfig();
        p_DisplaySystem = &displaySystem;
        p_SWRenderer = std::make_unique<SWRenderer>();
		p_GLRenderer = std::make_unique<GLRenderer>();
        if (!p_SWRenderer->Init(p_DisplaySystem->GetWidth(), p_DisplaySystem->GetHeight()))
        {
            LOGE("Failed to initialize SWRenderer");
            return false;
        }
		if (!p_GLRenderer->Init(p_DisplaySystem->GetWidth(), p_DisplaySystem->GetHeight()))
		{
			LOGE("Failed to initialize GLRenderer");
			return false;
		}

        p_Stats = stats;

		auto fbResolution = p_Config->renderer.resolution;
		CreateFramebufferTexture(m_SWFramebufferTexture, fbResolution.x, fbResolution.y);
		CreateFramebufferTexture(m_GLFramebufferTexture, fbResolution.x, fbResolution.y);

        return true;
    }

	void RenderSystem::BeforeFrame(Uint32 clearColor)
	{
        // TODO: utility function?
		Uint32 a = (clearColor & 0xFF000000) >> 24;
		Uint32 b = (clearColor & 0x00FF0000) >> 16;
		Uint32 g = (clearColor & 0x0000FF00) >> 8;
		Uint32 r = (clearColor & 0x000000FF);
        Uint32 argbColor = (a << 24) | (r << 16) | (g << 8) | b;
		p_SWRenderer->GetRenderTarget().Clear(argbColor);
	}

    std::queue<Model*>& RenderSystem::BuildRenderQueue(Scene& scene, const Camera& camera)
    {
        auto &activeRenderer = p_SWRenderer; // TODO: get from config
        activeRenderer->SetActiveCamera(camera);
        return scene.GetVisibleModels(); // TODO: split into meshes?
    }

    GLuint RenderSystem::Render(std::queue<Model*>& renderQueue)
    {
		auto p_Config = Engine::Get().GetConfig();
        auto selectedRenderer = p_Config->renderer.selectedRenderer;
		IRenderer* activeRenderer = nullptr;
		GLuint fbTex = 0;
		switch (selectedRenderer)
		{
		case Config::RendererType::SOFTWARE:
			activeRenderer = p_SWRenderer.get();
			fbTex = m_SWFramebufferTexture;
			break;
		case Config::RendererType::GL:
			activeRenderer = p_GLRenderer.get();
			fbTex = m_GLFramebufferTexture;
			break;
		}
        assert(activeRenderer != nullptr && "Active renderer is null");
		assert(p_Stats != nullptr && "Stats not initialized!");
		p_Stats->Reset();

		//LOGD("Render queue size: %d", renderQueue.size());
        while (!renderQueue.empty())
        {
            const Model* model = renderQueue.front();
			assert(model != nullptr && "Model is null");
			activeRenderer->DrawTriangularMesh(model);
            renderQueue.pop();
        }

        glBindTexture(GL_TEXTURE_2D, fbTex);
        glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0,
			0,
			p_SWRenderer->GetRenderTarget().width,
			p_SWRenderer->GetRenderTarget().height,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			p_SWRenderer->GetRenderTarget().data
		);
        glBindTexture(GL_TEXTURE_2D, 0);
        return fbTex;
    }

    /**
	* @brief Fill the framebuffer with random color for testing whether framebuffer texture is working.
	* @return GLuint framebuffer texture handle
	*/
    GLuint RenderSystem::TestFill()
	{
		const int rangeFrom = 0x0;
		const int rangeTo = 0xFFFFFFFF;
        std::random_device randDev;
        std::mt19937 generator(randDev());
        std::uniform_int_distribution<uint32_t>distr(rangeFrom, rangeTo);
		p_SWRenderer->GetRenderTarget().Clear(distr(generator));

		glBindTexture(GL_TEXTURE_2D, m_SWFramebufferTexture);
		glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0,
			0,
			p_SWRenderer->GetRenderTarget().width,
			p_SWRenderer->GetRenderTarget().height,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			p_SWRenderer->GetRenderTarget().data
		);
		glBindTexture(GL_TEXTURE_2D, 0);
        return m_SWFramebufferTexture;
    }

	void RenderSystem::Resize(const glm::ivec2& resolution)
	{
		auto& p_Config = Engine::Get().GetConfig();
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

    void RenderSystem::OnLoadScene(const SceneLoadEvent& e)
    {

    }

}