#pragma once
#ifndef __ANDROID__
    #include <glad/glad.h>
#endif
#include "../../Scene/Camera.h"
#include "../../Scene/Scene.h"
#include "../IRenderer.h"
#include "../../Base/Color.h"
#include "../../Scene/MaterialManager.h"

namespace RetroRenderer
{
    class GLRenderer : public IRenderer
    {
    public:
        GLRenderer() = default;

        ~GLRenderer() = default;

        bool Init(GLuint fbTex, int w, int h);

        void Resize(GLuint newFbTex, int w, int h);

        void Destroy() override;

        void SetActiveCamera(const Camera &camera) override;

        void DrawTriangularMesh(const Model *model) override;

        void DrawSkybox() override;

        void BeforeFrame(const Color &clearColor) override;

        GLuint EndFrame() override;

        GLuint CreateShaderProgram();
        GLuint CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) override;

    private:
        static void
        DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
                      const void *userParam);
        bool CreateFramebuffer(GLuint fbTex, int w, int h);
        GLuint CompileShader(GLenum shaderType, const char *shaderSource);
        void CheckShaderErrors(GLuint shader, const std::string &type);
        void CreateFallbackTexture();
        GLuint CreateCubemap(const std::string& path);

    private:
        Camera *p_Camera = nullptr;

        GLuint p_FrameBufferTexture = 0;
        GLuint m_FrameBuffer = 0;
        GLuint m_DepthBuffer = 0;
        GLuint m_FallbackTexture = 0;
        GLuint m_SkyboxTexture = 0;
        ShaderProgram m_SkyboxProgram;
    };
}