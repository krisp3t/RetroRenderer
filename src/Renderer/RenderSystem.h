#pragma once

#include <memory>
#include <GLAD/glad.h>
#include "../Window/DisplaySystem.h"
#include "../Base/Event.h"
#include "../Base/Stats.h"
#include "../Scene/Scene.h"
#include "Software/SWRenderer.h"
#include "OpenGL/GLRenderer.h"
#include "../Base/Color.h"

namespace RetroRenderer
{

    class RenderSystem
    {
    public:
        RenderSystem() = default;

        ~RenderSystem() = default;

        static void CreateFramebufferTexture(GLuint &texId, int width, int height);

        bool Init(SDL_Window *window, std::shared_ptr<Stats> stats);

        void BeforeFrame(const Color &clearColor);

        [[nodiscard]] std::vector<int> &BuildRenderQueue(Scene &scene, const Camera &camera);

        GLuint Render(std::vector<int> &renderQueue, std::vector<Model> &models);

        void Resize(const glm::ivec2 &resolution);

        void Destroy();

        void OnLoadScene(const SceneLoadEvent &e);

    private:
        std::unique_ptr<Scene> p_Scene = nullptr;
        std::unique_ptr<SWRenderer> p_SWRenderer = nullptr;
        std::unique_ptr<GLRenderer> p_GLRenderer = nullptr;
        std::shared_ptr<Stats> p_Stats = nullptr;
        IRenderer *p_activeRenderer = nullptr;

        GLuint m_SWFramebufferTexture = 0;
        GLuint m_GLFramebufferTexture = 0;
    };

}


