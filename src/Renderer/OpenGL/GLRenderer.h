#pragma once

#include <glad/glad.h>
#include "../../Scene/Camera.h"
#include "../../Scene/Scene.h"
#include "../IRenderer.h"

namespace RetroRenderer
{
    class GLRenderer : public IRenderer
    {
    public:
        GLRenderer() = default;

        ~GLRenderer() = default;

        bool Init(SDL_Window *window, GLuint renderTarget, int w, int h);

        void Resize(int w, int h);

        void Destroy();

        void SetActiveCamera(const Camera &camera);

        void DrawTriangularMesh(const Model *model);

        void Render();

        GLuint GetRenderTarget();

    private:
        SDL_Window *m_Window = nullptr;
        Camera *p_Camera = nullptr;
        SDL_GLContext m_glContext = nullptr;

        GLuint m_VAO = 0;
        GLuint m_VBO = 0;
        GLuint m_EBO = 0;

        GLuint m_FrameBuffer = 0;
        GLuint m_DepthBuffer = 0;
        GLuint m_RenderTarget = 0;

        static void
        DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
                      const void *userParam);
    };
}