#include "GLRenderer.h"
#include "../../Base/Logger.h"

namespace RetroRenderer
{
    /**
     * @brief Debug callback for OpenGL errors. Set breakpoint to catch errors.
     */
    void APIENTRY GLRenderer::DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                            GLsizei length, const GLchar *message, const void *userParam)
    {
        // TODO: add line where error occurred
        if (severity == GL_DEBUG_SEVERITY_MEDIUM)
        {
            LOGW("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
                 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
                 type, severity, message);
        } else if (severity == GL_DEBUG_SEVERITY_HIGH)
        {
            LOGE("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
                 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
                 type, severity, message);
        } else
        {
            LOGD("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
                 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
                 type, severity, message);
        }
    }

    bool GLRenderer::Init(GLuint fbTex, int w, int h)
    {
        // Enable Debug Output
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        // TODO: check if extension supported
        glDebugMessageCallback(DebugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);

        if (!CreateFramebuffer(fbTex, w, h))
        {
            return false;
        }

        // TODO: remove
        auto shaderProgram = CreateShaderProgram();
        glUseProgram(shaderProgram);

        // TODO: add depth buffer
        glViewport(0, 0, w, h);
        return true;
    }

    /**
     * @brief Create a framebuffer for rendering to a texture.
     * @param fbTex
     * @return
     */
    bool GLRenderer::CreateFramebuffer(GLuint fbTex, int w, int h)
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

    void GLRenderer::Resize(GLuint newFbTex, int w, int h)
    {
        p_FrameBufferTexture = newFbTex;
        glViewport(0, 0, w, h);
        CreateFramebuffer(p_FrameBufferTexture, w, h);
    }

    void GLRenderer::Destroy()
    {
        //glDeleteFramebuffers(1, &m_FrameBuffer);
        //glDeleteTextures(1, &m_RenderTexture);
        //glDeleteRenderbuffers(1, &m_DepthBuffer);
    }

    void GLRenderer::SetActiveCamera(const Camera &camera)
    {
        p_Camera = const_cast<Camera *>(&camera);
    }

    void GLRenderer::DrawTriangularMesh(const Model *model)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
        //SDL_GL_MakeCurrent(m_Window, m_glContext);
        float vertices[] = {
                -0.5f, -0.5f, 0.0f,
                0.5f, -0.5f, 0.0f,
                0.0f, 0.5f, 0.0f
        };


        unsigned int VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLRenderer::BeforeFrame(const Color &clearColor)
    {
        auto c = clearColor.ToImVec4();
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
        glClearColor(c.x, c.y, c.z, c.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint GLRenderer::EndFrame()
    {
        return p_FrameBufferTexture;
    }

    GLuint GLRenderer::CreateShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource)
    {
        return 0;
    }

    /**
     * Default shader program (if no shader program is provided)
     */
    GLuint GLRenderer::CreateShaderProgram()
    {
        const char *vertexShaderSource = "#version 330 core\n"
                                         "layout (location = 0) in vec3 aPos;\n"
                                         "void main()\n"
                                         "{\n"
                                         "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                         "}\0";
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);
        const char *fragmentShaderSource = "#version 330 core\n"
                                           "out vec4 FragColor;\n"
                                           "void main()\n"
                                           "{\n"
                                           "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                           "}\n\0";
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);
        GLuint shaderProgram;
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glUseProgram(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return shaderProgram;
    }

}