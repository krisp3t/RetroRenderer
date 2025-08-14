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

        // TODO: add depth buffer
        CreateFallbackTexture();
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

        // Allocate color texture storage
        glBindTexture(GL_TEXTURE_2D, p_FrameBufferTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create framebuffer
        glGenFramebuffers(1, &m_FrameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_FrameBufferTexture, 0);

        // Create depth+stencil buffer
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
        // TODO: Cache uniforms after shader compile?
        MaterialManager::Material& mat = Engine::Get().GetMaterialManager().GetCurrentMaterial();
        glUseProgram(mat.shaderProgram.id);

        const glm::mat4& modelMat = model->GetTransform();
        const glm::mat4& viewMat = p_Camera->m_ViewMat;
        const glm::mat4& projMat = p_Camera->m_ProjMat;
        const glm::mat4 mv = viewMat * modelMat;
        const glm::mat4 mvp = projMat * mv;
        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMat)));
        GLint normalMatLoc = glGetUniformLocation(mat.shaderProgram.id, "u_NormalMatrix");
        glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

        // Per-mesh uniforms
        GLint modelLoc = glGetUniformLocation(mat.shaderProgram.id, "u_ModelMatrix");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
        GLint viewProjLoc = glGetUniformLocation(mat.shaderProgram.id, "u_ViewProjectionMatrix");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glUniform3f(glGetUniformLocation(mat.shaderProgram.id, "u_LightPos"), 5.0f, 5.0f, 5.0f);
        glUniform3f(glGetUniformLocation(mat.shaderProgram.id, "u_ViewPos"), p_Camera->m_Position.x, p_Camera->m_Position.y, p_Camera->m_Position.z);
        glUniform3f(glGetUniformLocation(mat.shaderProgram.id, "u_LightColor"), 1.0f, 1.0f, 1.0f);
        GLint texLoc = glGetUniformLocation(mat.shaderProgram.id, "u_Texture");

        auto& meshes = model->GetMeshes();
        for (const Mesh& mesh : meshes)
        {
            // TODO: replace with per-mesh texture?
            if (mat.texture.GetID() == 0)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_FallbackTexture);
            }
            else
            {
                mat.texture.Bind();
            }
            glUniform1i(texLoc, 0);
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.m_Indices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);
    }

    void GLESRenderer::BeforeFrame(const Color& clearColor)
    {
        auto c = clearColor.ToImVec4();
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
        glClearColor(c.x, c.y, c.z, c.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    GLuint
    GLESRenderer::CompileShaders(const std::string &vertexCode, const std::string &fragmentCode) {
        GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode.c_str());
        if (vertexShader == 0)
        {
            return 0;
        }
        CheckShaderErrors(vertexShader, "VERTEX");
        GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());
        if (fragmentShader == 0)
        {
            glDeleteShader(vertexShader);
            return 0;
        }
        CheckShaderErrors(fragmentShader, "FRAGMENT");

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
        CheckShaderErrors(shaderProgram, "PROGRAM");

        // TODO: get rid of duplicate check shader errors
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
        LOGD("GLESRenderer: Compiled shader: %s", shaderSource);
        return shader;
    }

    void GLESRenderer::CheckShaderErrors(GLuint shader, const std::string &type) {
        GLint success;
        if (type == "PROGRAM")
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (success == GL_FALSE)
            {
                GLint logLength;
                glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                std::vector<char> errorLog(logLength);
                glGetProgramInfoLog(shader, logLength, &logLength, errorLog.data());
                LOGE("Error linking %s shader: %s", type.c_str(), errorLog.data());
            }
        } else
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (success == GL_FALSE)
            {
                GLint logLength;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                std::vector<char> errorLog(logLength);
                glGetShaderInfoLog(shader, logLength, &logLength, errorLog.data());
                LOGE("Error compiling %s shader: %s", type.c_str(), errorLog.data());
            }
        }
    }

    void GLESRenderer::CreateFallbackTexture() {
        unsigned char whitePixel[4] = {255, 255, 255, 255};

        glGenTextures(1, &m_FallbackTexture);
        glBindTexture(GL_TEXTURE_2D, m_FallbackTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    void GLESRenderer::DrawSkybox()
    {
    }
}
