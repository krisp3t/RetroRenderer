#include "GLRenderer.h"
#include "../../Base/Logger.h"

namespace RetroRenderer
{
    void APIENTRY GLRenderer::DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                            GLsizei length, const GLchar *message, const void *userParam)
    {
        LOGD("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
             (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
             type, severity, message);
    }

    bool GLRenderer::Init(SDL_Window *window, int w, int h)
    {
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1); // enable resource sharing with main context
        m_glContext = SDL_GL_CreateContext(window);
        if (!m_glContext)
        {
            LOGE("Failed to create GL context: %s", SDL_GetError());
            return false;
        }
        m_Window = window;
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        // TODO: check if extension supported
        glDebugMessageCallback(DebugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
        glViewport(0, 0, w, h);
        return true;
    }

    void GLRenderer::Resize(int w, int h)
    {
        glViewport(0, 0, w, h);
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
        SDL_GL_MakeCurrent(m_Window, m_glContext);
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


        const char *vertexShaderSource = "#version 330 core\n"
                                         "layout (location = 0) in vec3 aPos;\n"
                                         "void main()\n"
                                         "{\n"
                                         "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                         "}\0";
        unsigned int vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        const char *fragmentShaderSource = "#version 330 core\n"
                                           "out vec4 FragColor;\n"
                                           "void main()\n"
                                           "{\n"
                                           "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                           "}\n\0";
        unsigned int fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        unsigned int shaderProgram;
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glUseProgram(shaderProgram);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

}