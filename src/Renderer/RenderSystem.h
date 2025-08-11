#pragma once
#include <memory>
#include "../Window/DisplaySystem.h"
#include "../Base/Event.h"
#include "../Base/Stats.h"
#include "../Scene/Scene.h"
#include "Software/SWRenderer.h"
#include "../Base/Color.h"
#ifdef __ANDROID__
	#include "GLES/GLESRenderer.h"
#else
	#include "OpenGL/GLRenderer.h"
#endif


namespace RetroRenderer
{

    class RenderSystem
    {
    public:
        RenderSystem() = default;

        ~RenderSystem() = default;

        static void CreateFramebufferTexture(GLuint &texId, int width, int height);

        bool Init(SDL_Window *window);

        void BeforeFrame(const Color &clearColor);

        [[nodiscard]] std::vector<int> &BuildRenderQueue(Scene &scene, const Camera &camera);

        GLuint Render(std::vector<int> &renderQueue, std::vector<Model> &models);

        void Resize(const glm::ivec2 &resolution);

        void Destroy();

        void OnLoadScene(const SceneLoadEvent &e);

    private:
        std::unique_ptr<Scene> p_scene_ = nullptr;
        std::unique_ptr<SWRenderer> p_SWRenderer_ = nullptr;
#ifdef __ANDROID__
        std::unique_ptr<GLESRenderer> p_GLRenderer_ = nullptr;
#else
        std::unique_ptr<GLRenderer> p_GLRenderer_ = nullptr;
#endif
        IRenderer *p_activeRenderer_ = nullptr;

        GLuint m_SWFramebufferTexture = 0;
        GLuint m_GLFramebufferTexture = 0;
    };

}


