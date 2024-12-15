#pragma once

#include <glad/glad.h>
#include "../../Scene/Camera.h"
#include "../../Scene/Scene.h"
#include "../IRenderer.h"
#include "../../Base/Color.h"

namespace RetroRenderer
{
    class GLRenderer : public IRenderer
    {
    public:
        GLRenderer() = default;

        ~GLRenderer() = default;

        bool Init(SDL_Window *window, GLuint fbTex, int w, int h);

        void Resize(int w, int h);

        void Destroy();

        void SetActiveCamera(const Camera &camera);

        void DrawTriangularMesh(const Model *model);

        void BeforeFrame(const Color &clearColor) override;

        GLuint EndFrame() override;

    private:
        SDL_Window *m_Window = nullptr;
        Camera *p_Camera = nullptr;
        SDL_GLContext m_glContext = nullptr;


        GLuint m_VAO = 0;
        GLuint m_VBO = 0;
        GLuint m_EBO = 0;

        GLuint p_FrameBufferTexture = 0;
        GLuint m_FrameBuffer = 0;
        GLuint m_DepthBuffer = 0;

        static void
        DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
                      const void *userParam);
    };
}