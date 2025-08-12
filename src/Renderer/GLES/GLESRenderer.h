#pragma once
#include "../../Scene/Camera.h"
#include "../../Scene/Scene.h"
#include "../IRenderer.h"
#include "../../Base/Color.h"

namespace RetroRenderer
{
    class GLESRenderer : public IRenderer
    {
    public:
        GLESRenderer() = default;

        ~GLESRenderer() = default;

        bool Init(GLuint fbTex, int w, int h);

        void Resize(GLuint newFbTex, int w, int h);

        void Destroy() override;

        void SetActiveCamera(const Camera& camera) override;

        void DrawTriangularMesh(const Model* model) override;

        void BeforeFrame(const Color& clearColor) override;

        GLuint EndFrame() override;

        GLuint CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) override;

    private:
        bool CreateFramebuffer(GLuint fbTex, int w, int h);
        GLuint CompileShader(GLenum shaderType, const char *shaderSource);
        void CheckShaderErrors(GLuint shader, const std::string &type);
        void CreateFallbackTexture();

    private:
        Camera* p_Camera = nullptr;

        GLuint m_FallbackTexture = 0;
        GLuint p_FrameBufferTexture = 0;
        GLuint m_FrameBuffer = 0;
        GLuint m_DepthBuffer = 0;
    };
}