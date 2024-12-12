#include "GLRenderer.h"
#include "../../Base/Logger.h"

namespace RetroRenderer
{
	bool GLRenderer::Init(int w, int h)
	{
		/*
		glGenFramebuffers(1, &m_FrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

		glGenTextures(1, &m_RenderTexture);
		glBindTexture(GL_TEXTURE_2D, m_RenderTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RenderTexture, 0);

		glGenRenderbuffers(1, &m_DepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			LOGE("Framebuffer is not complete!");
			return false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		*/
		return true;
	}

	void GLRenderer::Destroy()
	{
		//glDeleteFramebuffers(1, &m_FrameBuffer);
		//glDeleteTextures(1, &m_RenderTexture);
		//glDeleteRenderbuffers(1, &m_DepthBuffer);
	}

	void GLRenderer::SetActiveCamera(const Camera& camera)
	{
		p_Camera = const_cast<Camera*>(&camera);
	}

	void GLRenderer::DrawTriangularMesh(const Model* model)
	{

	}

}