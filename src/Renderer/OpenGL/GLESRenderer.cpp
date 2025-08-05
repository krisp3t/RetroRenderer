#include <glm/gtc/type_ptr.hpp>
#include <KrisLogger/Logger.h>
#include "GLESRenderer.h"
#include "../../Base/Debug.h"
#include "../../Engine.h"

namespace RetroRenderer
{
    bool GLESRenderer::Init(GLuint fbTex, int w, int h)
    {
        if (!CreateFramebuffer(fbTex, w, h))
        {
            return false;
        }

        // TODO: remove
        const char* vertexShaderSource = R"glsl(
#version 300 es
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewProjectionMatrix;

out vec3 FragPos;
out vec2 TexCoord;

void main() {
    FragPos = vec3(u_ModelMatrix * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    gl_Position = u_ViewProjectionMatrix * vec4(FragPos, 1.0);
}
)glsl";
        const char* fragmentShaderSource = R"glsl(
#version 300 es
in vec3 FragPos;
in vec2 TexCoord;

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)glsl";
        m_ShaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
        glUseProgram(m_ShaderProgram);

        // TODO: add depth buffer
        glViewport(0, 0, w, h);
        return true;
    }

    /**
     * @brief Create a framebuffer for rendering to a texture.
     * @param fbTex
     * @return
     */
    bool GLESRenderer::CreateFramebuffer(GLuint fbTex, int w, int h)
    {
        p_FrameBufferTexture = fbTex;
        glGenFramebuffers(1, &m_FrameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_FrameBufferTexture, 0);
        glGenRenderbuffers(1, &m_DepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            LOGE("Framebuffer is not complete!");
            return false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    void GLESRenderer::Resize(GLuint newFbTex, int w, int h)
    {
        p_FrameBufferTexture = newFbTex;
        glViewport(0, 0, w, h);
        CreateFramebuffer(p_FrameBufferTexture, w, h);
    }

    void GLESRenderer::Destroy()
    {
        //glDeleteFramebuffers(1, &m_FrameBuffer);
        //glDeleteTextures(1, &m_RenderTexture);
        //glDeleteRenderbuffers(1, &m_DepthBuffer);
    }

    void GLESRenderer::SetActiveCamera(const Camera& camera)
    {
        p_Camera = const_cast<Camera*>(&camera);
    }

    void GLESRenderer::DrawTriangularMesh(const Model* model)
    {
        const glm::mat4& modelMat = model->GetTransform();
        const glm::mat4& viewMat = p_Camera->viewMat;
        const glm::mat4& projMat = p_Camera->projMat;
        const glm::mat4 mv = viewMat * modelMat;
        const glm::mat4 mvp = projMat * mv;
        const glm::mat4 n = glm::transpose(glm::inverse(modelMat));

        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

        // Per-mesh uniforms
        GLint modelLoc = glGetUniformLocation(m_ShaderProgram, "u_ModelMatrix");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
        GLint viewProjLoc = glGetUniformLocation(m_ShaderProgram, "u_ViewProjectionMatrix");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        auto& meshes = model->GetMeshes();
        for (const Mesh& mesh : meshes)
        {
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.m_Indices.size(), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glUseProgram(0);
    }

    void GLESRenderer::BeforeFrame(const Color& clearColor)
    {
        auto c = clearColor.ToImVec4();
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
        glClearColor(c.x, c.y, c.z, c.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        auto& config = Engine::Get().GetConfig();
        /*
        switch (config->gl.rasterizer.polygonMode)
        {
        case Config::RasterizationPolygonMode::POINT:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case Config::RasterizationPolygonMode::LINE:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case Config::RasterizationPolygonMode::FILL:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        }
         */
    }

    GLuint GLESRenderer::EndFrame()
    {
        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        return p_FrameBufferTexture;
    }


    GLuint GLESRenderer::CompileShader(GLenum shaderType, const char* shaderSource)
    {
        GLuint shader = glCreateShader(shaderType);
        if (shader == 0)
        {
            LOGE("Error creating shader");
            return 0;
        }
        glShaderSource(shader, 1, &shaderSource, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> errorLog(logLength);
            glGetShaderInfoLog(shader, logLength, &logLength, errorLog.data());
            LOGE("Error compiling shader: %s", errorLog.data());
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint GLESRenderer::CreateShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
    {
        GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
        if (vertexShader == 0)
        {
            return 0;
        }

        GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
        if (fragmentShader == 0)
        {
            glDeleteShader(vertexShader);
            return 0;
        }

        GLuint shaderProgram = glCreateProgram();
        if (shaderProgram == 0)
        {
            LOGE("Error creating shader program");
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return 0;
        }

        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);


        glLinkProgram(shaderProgram);
        GLint linkStatus;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
        if (linkStatus == GL_FALSE)
        {
            GLint logLength;
            glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> errorLog(logLength);
            glGetProgramInfoLog(shaderProgram, logLength, &logLength, errorLog.data());
            LOGE("Error linking shader program: %s", errorLog.data());
            glDeleteProgram(shaderProgram);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return 0;
        }

        glDetachShader(shaderProgram, vertexShader);
        glDetachShader(shaderProgram, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

    /**
     * Default shader program (if no shader program is provided). Used for debugging purposes.
     */
    GLuint GLESRenderer::CreateShaderProgram()
    {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "out vec4 vertexColor;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "   vertexColor = vec4(1.0, 0.5, 0.0, 1.0);\n"
            "}\0";
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);
        const char* fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "in vec4 vertexColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vertexColor;\n"
            "}\n\0";
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);
        GLuint shaderProgram;
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return shaderProgram;
    }

}