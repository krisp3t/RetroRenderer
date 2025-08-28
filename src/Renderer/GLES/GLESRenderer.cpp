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

        CreateFallbackTexture();
        GLuint cubeTex = CreateCubemap("img/skybox-cubemap/Cubemap_Sky_23-512x512.png");
        if (cubeTex != 0)
        {
            m_SkyboxTexture = cubeTex;
            auto skyboxShader = MaterialManager::CreateShaderProgram("shaders/skybox.vs",
                                                                     "shaders/skybox.fs");
            m_SkyboxProgram = skyboxShader;
            m_SkyboxVAO = CreateSkyboxVAO();
        }
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
        auto& config = Engine::Get().GetConfig();
        glUseProgram(mat.shaderProgram.id);

        const glm::vec3 lightPos = config->environment.lightPosition;
        const glm::mat4& modelMat = model->GetWorldTransform();
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

        // TODO: replace with PhongParamsUBO
        glUniform3f(glGetUniformLocation(mat.shaderProgram.id, "u_LightPos"), lightPos.x, lightPos.y, lightPos.z);
        // TODO: dynamic lights
        glUniform3f(glGetUniformLocation(mat.shaderProgram.id, "u_ViewPos"), p_Camera->m_Position.x,
                    p_Camera->m_Position.y, p_Camera->m_Position.z);
        glUniform3f(glGetUniformLocation(mat.shaderProgram.id, "u_LightColor"), mat.lightColor.r, mat.lightColor.g,
                    mat.lightColor.b);
        if (mat.phongParams.has_value())
        {
            glUniform1f(glGetUniformLocation(mat.shaderProgram.id, "u_Shininess"), mat.phongParams->shininess);
            glUniform1f(glGetUniformLocation(mat.shaderProgram.id, "u_AmbientStrength"),
                        mat.phongParams->ambientStrength);
            glUniform1f(glGetUniformLocation(mat.shaderProgram.id, "u_SpecularStrength"),
                        mat.phongParams->specularStrength);
        }
        GLint texLoc = glGetUniformLocation(mat.shaderProgram.id, "u_Texture");

        auto& meshes = model->GetMeshes();
        for (const Mesh& mesh : meshes)
        {
            // TODO: replace with per-mesh texture?
            if (!mat.texture.has_value() || mat.texture->GetID() == 0)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_FallbackTexture);
            }
            else
            {
                mat.texture->Bind();
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
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint GLESRenderer::EndFrame()
    {
        return p_FrameBufferTexture;
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

    GLuint GLESRenderer::CompileShader(GLenum shaderType, const char* shaderSource)
    {
        constexpr std::string_view kShaderPrefix = "#version 300 es\nprecision highp float;\n";
        std::string appendStr = std::string(kShaderPrefix) + shaderSource;
        const char* srcPtr = appendStr.c_str();

        GLuint shader = glCreateShader(shaderType);
        if (shader == 0)
        {
            LOGE("Error creating shader");
            return 0;
        }
        glShaderSource(shader, 1, &srcPtr, nullptr);
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
        LOGD("GLESRenderer: Compiled shader: %s", srcPtr);
        return shader;
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

    void GLESRenderer::CreateFallbackTexture() {
        unsigned char whitePixel[4] = {255, 255, 255, 255};

        glGenTextures(1, &m_FallbackTexture);
        glBindTexture(GL_TEXTURE_2D, m_FallbackTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    /**
     * Create cubemap in horizontal strip format.
     * @param path Path of cubemap.
     */
    GLuint GLESRenderer::CreateCubemap(const std::string& path)
    {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface)
        {
            LOGW("Could not load cubemap %s!", path.c_str());
            return 0;
        }

        int fullWidth = surface->w;
        int fullHeight = surface->h;
        int faceSize = fullWidth / 4; // horizontal cross: 4 faces in middle row

        if (fullHeight != faceSize * 3)
        {
            LOGE("Invalid cubemap cross layout dimensions: %dx%d", fullWidth, fullHeight);
            SDL_FreeSurface(surface);
            return 0;
        }

        GLuint cubemapTex;
        glGenTextures(1, &cubemapTex);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);

        int bpp = surface->format->BytesPerPixel;
        Uint8* pixels = (Uint8*)surface->pixels;

        struct Offset
        {
            int x, y;
        };
        Offset faceOffsets[6] = {
                {2, 1}, // +X
                {0, 1}, // -X
                {1, 0}, // +Y
                {1, 2}, // -Y
                {1, 1}, // +Z
                {3, 1} // -Z
        };

        for (int face = 0; face < 6; ++face)
        {
            int ox = faceOffsets[face].x * faceSize;
            int oy = faceOffsets[face].y * faceSize;

            Uint8* facePixels = new Uint8[faceSize * faceSize * bpp];

            for (int row = 0; row < faceSize; ++row)
            {
                memcpy(
                        facePixels + row * faceSize * bpp,
                        pixels + (oy + row) * fullWidth * bpp + ox * bpp,
                        faceSize * bpp
                );
            }

            GLenum format = (bpp == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                    0, format, faceSize, faceSize, 0,
                    format, GL_UNSIGNED_BYTE, facePixels
            );

            delete[] facePixels;
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        LOGI("Created skybox %s with texture handle %d", path.c_str(), cubemapTex);

        SDL_FreeSurface(surface);
        return cubemapTex;
    }

    void GLESRenderer::DrawSkybox()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glUseProgram(m_SkyboxProgram.id);

        glm::mat4 view = glm::mat4(glm::mat3(p_Camera->m_ViewMat)); // remove translation
        glm::mat4 proj = p_Camera->m_ProjMat;

        GLint viewLoc = glGetUniformLocation(m_SkyboxProgram.id, "u_ViewMatrix");
        GLint projLoc = glGetUniformLocation(m_SkyboxProgram.id, "u_ProjectionMatrix");
        glUniform1i(glGetUniformLocation(m_SkyboxProgram.id, "skybox"), 0);


        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

        glBindVertexArray(m_SkyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTexture);

        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 vertices for a cube

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint GLESRenderer::CreateSkyboxVAO()
    {
        static const float kSkyboxVerts[] = {
                -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,

                -1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,

                1.0f, -1.0f, -1.0f,
                1.0f, -1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,

                -1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,

                -1.0f,  1.0f, -1.0f,
                1.0f,  1.0f, -1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,

                -1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                1.0f, -1.0f,  1.0f
        };
        GLuint vao = 0;
        GLuint vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(kSkyboxVerts), kSkyboxVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
        return vao;
    }
}
