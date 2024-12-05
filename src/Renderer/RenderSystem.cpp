#include <cassert>
#include <iostream>
#include <random>
#include "RenderSystem.h"
#include "../Base/Logger.h"

namespace RetroRenderer
{
    bool RenderSystem::Init(DisplaySystem& displaySystem)
    {
        p_DisplaySystem = &displaySystem;
        p_SWRenderer = std::make_unique<SWRenderer>();
        if (!p_SWRenderer->Init(p_DisplaySystem->GetWidth(), p_DisplaySystem->GetHeight()))
        {
            LOGE("Failed to initialize SWRenderer");
            return false;
        }

        LOGD("SWRenderer initialized");

        glGenTextures(1, &m_framebufferTexture);
		glBindTexture(GL_TEXTURE_2D, m_framebufferTexture);
		glTexImage2D(
            GL_TEXTURE_2D, 
            0, 
            GL_RGBA, 
            p_SWRenderer->GetRenderTarget().width, 
            p_SWRenderer->GetRenderTarget().height, 
            0, 
            GL_RGBA, 
            GL_UNSIGNED_BYTE, 
            nullptr
        );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

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
        return scene.GetVisibleModels();
    }

    void RenderSystem::Render(std::queue<Model *>& renderQueue)
    {
        auto &activeRenderer = p_SWRenderer; // TODO: get from config
        assert(activeRenderer != nullptr && "Active renderer is null");

        while (!renderQueue.empty())
        {
			//LOGD("Render queue size: %d", renderQueue.size());
            auto model = renderQueue.front();
            activeRenderer->DrawTriangularMesh(*model->GetMesh());
            renderQueue.pop();
        }

        glBindTexture(GL_TEXTURE_2D, m_framebufferTexture);
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

		glBindTexture(GL_TEXTURE_2D, m_framebufferTexture);
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
        return m_framebufferTexture;
    }

    void RenderSystem::Destroy()
    {
    }

    void RenderSystem::OnLoadScene(const SceneLoadEvent& e)
    {

    }

}