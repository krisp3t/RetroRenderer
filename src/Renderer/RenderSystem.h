#pragma once

#include <memory>
#include <GLAD/glad.h>
#include "../Window/DisplaySystem.h"
#include "../Base/Event.h"
#include "../Base/Stats.h"
#include "../Scene/Scene.h"
#include "Software/SWRenderer.h"

namespace RetroRenderer
{

	class RenderSystem
	{
	public:
		RenderSystem() = default;
		~RenderSystem() = default;

		bool Init(DisplaySystem& displaySystem, std::shared_ptr<Stats> stats);
		void BeforeFrame(Uint32 clearColor);
		std::queue<Model*>& BuildRenderQueue(Scene& scene, const Camera& camera);
		GLuint Render(std::queue<Model*>& renderQueue);
		GLuint TestFill();
		void Destroy();
		void OnLoadScene(const SceneLoadEvent& e);
	private:
		DisplaySystem* p_DisplaySystem = nullptr;

		std::unique_ptr<Scene> p_Scene = nullptr;
		std::unique_ptr<SWRenderer> p_SWRenderer = nullptr;
		std::shared_ptr<Stats> p_Stats = nullptr;

		GLuint m_framebufferTexture = 0;
	};

}


