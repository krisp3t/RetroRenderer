#include "GLRenderer.h"
#include "../GridGizmo.h"
#include "../../Base/Debug.h"
#include "../../Engine.h"
#include <KrisLogger/Logger.h>
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace RetroRenderer {
namespace {
GLuint ToGLHandle(ShaderHandle handle) {
    return static_cast<GLuint>(handle.value);
}

GLuint ToGLHandle(TextureHandle handle) {
    return static_cast<GLuint>(handle.value);
}
} // namespace

/**
 * @brief Debug callback for OpenGL errors. Set breakpoint to catch errors.
 */
void APIENTRY GLRenderer::DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                        const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        LOGW("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
             (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
    } else if (severity == GL_DEBUG_SEVERITY_HIGH) {
        LOGE("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
             (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
    } else {
        LOGD("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
             (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
    }
#ifndef NDEBUG
    DEBUG_BREAK(); // Traverse across callstack to find error
#endif
}

bool GLRenderer::Init(TextureHandle fbTex, int w, int h) {
    // Enable Debug Output
    if (glDebugMessageCallback == nullptr) {
        LOGW("glDebugMessageCallback not supported on this platform");
    } else {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(DebugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
        LOGD("OpenGL debug context initialized");
    }
    if (!CreateFramebuffer(fbTex, w, h)) {
        return false;
    }

    CreateFallbackTexture();
    GLuint cubeTex = CreateCubemap("assets/img/skybox-cubemap/Cubemap_Sky_23-512x512.png");
    if (cubeTex != 0) {
        m_SkyboxTexture = cubeTex;
        auto skyboxShader = MaterialManager::CreateShaderProgram("shaders/skybox.vs", "shaders/skybox.fs");
        m_SkyboxProgram = skyboxShader;
        m_SkyboxVAO = CreateSkyboxVAO();
    }

    constexpr const char* kGridVertexShader =
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "uniform mat4 u_MVP;\n"
        "out vec3 vColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = u_MVP * vec4(aPos, 1.0);\n"
        "   vColor = aColor;\n"
        "}\n";
    constexpr const char* kGridFragmentShader =
        "in vec3 vColor;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(vColor, 1.0);\n"
        "}\n";
    m_GridProgram = CompileShaders(kGridVertexShader, kGridFragmentShader);
    if (m_GridProgram.IsValid()) {
        struct GridVertexGpu {
            glm::vec3 position;
            glm::vec3 color;
        };
        glGenVertexArrays(1, &m_GridVAO);
        glGenBuffers(1, &m_GridVBO);
        glBindVertexArray(m_GridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertexGpu), reinterpret_cast<void*>(offsetof(GridVertexGpu, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertexGpu), reinterpret_cast<void*>(offsetof(GridVertexGpu, color)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glViewport(0, 0, w, h);
    return true;
}

/**
 * @brief Create a framebuffer for rendering to a texture.
 * @param fbTex
 * @return
 */
bool GLRenderer::CreateFramebuffer(TextureHandle fbTex, int w, int h) {
    DestroyFramebufferResources();
    p_FrameBufferTexture = ToGLHandle(fbTex);
    m_FrameWidth = w;
    m_FrameHeight = h;
    glGenFramebuffers(1, &m_FrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_FrameBufferTexture, 0);
    glGenRenderbuffers(1, &m_DepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer is not complete!");
        DestroyFramebufferResources();
        return false;
    }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void GLRenderer::Resize(TextureHandle newFbTex, int w, int h) {
    glViewport(0, 0, w, h);
    if (!CreateFramebuffer(newFbTex, w, h)) {
        LOGE("Failed to resize GL framebuffer to %d x %d", w, h);
    }
}

void GLRenderer::Destroy() {
    m_MeshResources.Clear();
    m_TextureResources.Clear();
    DestroyFramebufferResources();
    DestroyRendererResources();
}

void GLRenderer::DestroyFramebufferResources() {
    if (m_FrameBuffer != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &m_FrameBuffer);
        m_FrameBuffer = 0;
    }
    if (m_DepthBuffer != 0) {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glDeleteRenderbuffers(1, &m_DepthBuffer);
        m_DepthBuffer = 0;
    }
    p_FrameBufferTexture = 0;
    m_FrameWidth = 0;
    m_FrameHeight = 0;
}

void GLRenderer::DestroyRendererResources() {
    if (m_FallbackTexture != 0) {
        glDeleteTextures(1, &m_FallbackTexture);
        m_FallbackTexture = 0;
    }
    if (m_SkyboxTexture != 0) {
        glDeleteTextures(1, &m_SkyboxTexture);
        m_SkyboxTexture = 0;
    }
    if (m_SkyboxVBO != 0) {
        glDeleteBuffers(1, &m_SkyboxVBO);
        m_SkyboxVBO = 0;
    }
    if (m_SkyboxVAO != 0) {
        glDeleteVertexArrays(1, &m_SkyboxVAO);
        m_SkyboxVAO = 0;
    }
    if (m_SkyboxProgram.handle.IsValid()) {
        glDeleteProgram(ToGLHandle(m_SkyboxProgram.handle));
        m_SkyboxProgram = {};
    }
    if (m_GridVBO != 0) {
        glDeleteBuffers(1, &m_GridVBO);
        m_GridVBO = 0;
    }
    if (m_GridVAO != 0) {
        glDeleteVertexArrays(1, &m_GridVAO);
        m_GridVAO = 0;
    }
    if (m_GridProgram.IsValid()) {
        glDeleteProgram(ToGLHandle(m_GridProgram));
        m_GridProgram = {};
    }
}

void GLRenderer::InvalidateSceneResources() {
    m_MeshResources.Clear();
    m_TextureResources.Clear();
}

void GLRenderer::InvalidateTextureResources() {
    m_TextureResources.Clear();
}

TextureHandle GLRenderer::GetTextureHandle(const Texture& texture) {
    return TextureHandle{static_cast<uintptr_t>(m_TextureResources.GetOrCreate(texture))};
}

void GLRenderer::SetActiveCamera(const Camera& camera) {
    p_Camera = const_cast<Camera*>(&camera);
}

void GLRenderer::SetSceneLights(const std::vector<LightSnapshot>& lights) {
    m_SceneLights = lights;
}

void GLRenderer::DrawTriangularMesh(const Model* model) {
    // TODO: Cache uniforms after shader compile?
    MaterialManager::Material& mat = Engine::Get().GetMaterialManager().GetCurrentMaterial();
    const GLuint shaderProgram = ToGLHandle(mat.shaderProgram.handle);
    if (shaderProgram == 0) {
        return;
    }
    auto& config = Engine::Get().GetConfig();
    glUseProgram(shaderProgram);

    const glm::vec3 lightPos = !m_SceneLights.empty() ? m_SceneLights.front().position : config->environment.lightPosition;
    const glm::mat4& modelMat = model->GetWorldTransform();
    const glm::mat4& viewMat = p_Camera->m_ViewMat;
    const glm::mat4& projMat = p_Camera->m_ProjMat;

    // Combined matrices
    glm::mat4 mv = viewMat * modelMat;
    glm::mat4 mvp = projMat * mv;
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMat)));

    // Upload uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_ModelMatrix"), 1, GL_FALSE,
                       glm::value_ptr(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "u_NormalMatrix"), 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));

    glUniform3f(glGetUniformLocation(shaderProgram, "u_LightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "u_ViewPos"), p_Camera->m_Position.x, p_Camera->m_Position.y,
                p_Camera->m_Position.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "u_LightColor"), mat.lightColor.r, mat.lightColor.g,
                mat.lightColor.b);

    if (mat.phongParams.has_value()) {
        glUniform1f(glGetUniformLocation(shaderProgram, "u_Shininess"), mat.phongParams->shininess);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_AmbientStrength"), mat.phongParams->ambientStrength);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_SpecularStrength"),
                    mat.phongParams->specularStrength);
    }
    GLint texLoc = glGetUniformLocation(shaderProgram, "u_Texture");
    GLint normalMatLoc = glGetUniformLocation(shaderProgram, "u_NormalMatrix");
    glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Draw meshes
    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    auto& meshes = model->GetMeshes();
    for (const Mesh& mesh : meshes) {
        // TODO: replace with per-mesh texture?
        const GLuint materialTexture = mat.texture.has_value() ? m_TextureResources.GetOrCreate(*mat.texture) : 0;
        if (materialTexture == 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_FallbackTexture);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, materialTexture);
        }

        glUniform1i(texLoc, 0);
        const auto& gpuMesh = m_MeshResources.GetOrCreate(mesh);
        glBindVertexArray(gpuMesh.vao);
        glDrawElements(GL_TRIANGLES, gpuMesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void GLRenderer::DrawGridGizmo() {
    if (p_Camera == nullptr || !m_GridProgram.IsValid() || m_GridVAO == 0 || m_GridVBO == 0) {
        return;
    }
    const GLuint gridProgram = ToGLHandle(m_GridProgram);

    struct GridVertexGpu {
        glm::vec3 position;
        glm::vec3 color;
    };
    const std::vector<GridGizmoVertex> gridVertices = BuildGridGizmoVertices(p_Camera->m_Position);
    std::vector<GridVertexGpu> packedVertices;
    packedVertices.reserve(gridVertices.size());
    for (const GridGizmoVertex& vertex : gridVertices) {
        packedVertices.push_back({vertex.position, vertex.color});
    }
    m_GridVertexCount = static_cast<GLsizei>(packedVertices.size());
    if (m_GridVertexCount == 0) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    glUseProgram(gridProgram);
    const glm::mat4 mvp = p_Camera->m_ProjMat * p_Camera->m_ViewMat;
    glUniformMatrix4fv(glGetUniformLocation(gridProgram, "u_MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(m_GridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(packedVertices.size() * sizeof(GridVertexGpu)),
        packedVertices.data(),
        GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, m_GridVertexCount);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void GLRenderer::BeforeFrame(const Color& clearColor) {
    auto c = clearColor.ToImVec4();
    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    glViewport(0, 0, m_FrameWidth, m_FrameHeight);
    glClearColor(c.x, c.y, c.z, c.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto& config = Engine::Get().GetConfig();
    switch (config->gl.rasterizer.polygonMode) {
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
}

void GLRenderer::EndFrame() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GLRenderer::CheckShaderErrors(GLuint shader, const std::string& type) {
    GLint success;
    if (type == "PROGRAM") {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (success == GL_FALSE) {
            GLint logLength;
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> errorLog(logLength);
            glGetProgramInfoLog(shader, logLength, &logLength, errorLog.data());
            LOGE("Error linking %s shader: %s", type.c_str(), errorLog.data());
        }
    } else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> errorLog(logLength);
            glGetShaderInfoLog(shader, logLength, &logLength, errorLog.data());
            LOGE("Error compiling %s shader: %s", type.c_str(), errorLog.data());
        }
    }
}

GLuint GLRenderer::CompileShader(GLenum shaderType, const char* shaderSource) {
    constexpr std::string_view kShaderPrefix = "#version 330 core\nprecision highp float;\n";
    std::string appendStr = std::string(kShaderPrefix) + shaderSource;
    const char* srcPtr = appendStr.c_str();

    GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        LOGE("Error creating shader");
        return 0;
    }
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> errorLog(logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, errorLog.data());
        LOGE("Error compiling shader: %s", errorLog.data());
        glDeleteShader(shader);
        return 0;
    }
    LOGD("GLRenderer: Compiled shader: %s", srcPtr);
    return shader;
}

ShaderHandle GLRenderer::CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode.c_str());
    if (vertexShader == 0) {
        return {};
    }
    CheckShaderErrors(vertexShader, "VERTEX");
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return {};
    }
    CheckShaderErrors(fragmentShader, "FRAGMENT");

    GLuint shaderProgram = glCreateProgram();
    if (shaderProgram == 0) {
        LOGE("Error creating shader program");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return {};
    }

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    CheckShaderErrors(shaderProgram, "PROGRAM");

    // TODO: get rid of duplicate check shader errors
    GLint linkStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint logLength;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> errorLog(logLength);
        glGetProgramInfoLog(shaderProgram, logLength, &logLength, errorLog.data());
        LOGE("Error linking shader program: %s", errorLog.data());
        glDeleteProgram(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return {};
    }

    glDetachShader(shaderProgram, vertexShader);
    glDetachShader(shaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return ShaderHandle{static_cast<uintptr_t>(shaderProgram)};
}

/**
 * Default shader program (if no shader program is provided). Used for debugging purposes.
 */
GLuint GLRenderer::CreateShaderProgram() {
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

void GLRenderer::CreateFallbackTexture() {
    if (m_FallbackTexture != 0) {
        glDeleteTextures(1, &m_FallbackTexture);
        m_FallbackTexture = 0;
    }

    unsigned char whitePixel[4] = {255, 255, 255, 255};

    glGenTextures(1, &m_FallbackTexture);
    glBindTexture(GL_TEXTURE_2D, m_FallbackTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * Create cubemap in horizontal strip format.
 * @param path Path of cubemap.
 */
GLuint GLRenderer::CreateCubemap(const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        LOGW("Could not load cubemap %s!", path.c_str());
        return 0;
    }

    int fullWidth = surface->w;
    int fullHeight = surface->h;
    int faceSize = fullWidth / 4; // horizontal cross: 4 faces in middle row

    if (fullHeight != faceSize * 3) {
        LOGE("Invalid cubemap cross layout dimensions: %dx%d", fullWidth, fullHeight);
        SDL_FreeSurface(surface);
        return 0;
    }

    GLuint cubemapTex;
    glGenTextures(1, &cubemapTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);

    int bpp = surface->format->BytesPerPixel;
    Uint8* pixels = (Uint8*)surface->pixels;

    struct Offset {
        int x, y;
    };
    Offset faceOffsets[6] = {
        {2, 1}, // +X
        {0, 1}, // -X
        {1, 0}, // +Y
        {1, 2}, // -Y
        {1, 1}, // +Z
        {3, 1}  // -Z
    };

    for (int face = 0; face < 6; ++face) {
        int ox = faceOffsets[face].x * faceSize;
        int oy = faceOffsets[face].y * faceSize;

        Uint8* facePixels = new Uint8[faceSize * faceSize * bpp];

        for (int row = 0; row < faceSize; ++row) {
            memcpy(facePixels + row * faceSize * bpp, pixels + (oy + row) * fullWidth * bpp + ox * bpp, faceSize * bpp);
        }

        GLenum format = (bpp == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, format, faceSize, faceSize, 0, format, GL_UNSIGNED_BYTE,
                     facePixels);

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

void GLRenderer::DrawSkybox() {
    if (p_Camera == nullptr || m_FrameBuffer == 0 || !m_SkyboxProgram.handle.IsValid() || m_SkyboxVAO == 0 || m_SkyboxTexture == 0) {
        return;
    }
    const GLuint skyboxProgram = ToGLHandle(m_SkyboxProgram.handle);

    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glUseProgram(skyboxProgram);

    glm::mat4 view = glm::mat4(glm::mat3(p_Camera->m_ViewMat)); // remove translation
    glm::mat4 proj = p_Camera->m_ProjMat;

    GLint viewLoc = glGetUniformLocation(skyboxProgram, "u_ViewMatrix");
    GLint projLoc = glGetUniformLocation(skyboxProgram, "u_ProjectionMatrix");
    glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);

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

GLuint GLRenderer::CreateSkyboxVAO() {
    if (m_SkyboxVBO != 0) {
        glDeleteBuffers(1, &m_SkyboxVBO);
        m_SkyboxVBO = 0;
    }
    if (m_SkyboxVAO != 0) {
        glDeleteVertexArrays(1, &m_SkyboxVAO);
        m_SkyboxVAO = 0;
    }

    static const float kSkyboxVerts[] = {-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                                         1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

                                         -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
                                         -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

                                         1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                                         1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

                                         -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                                         1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

                                         -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
                                         1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

                                         -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
                                         1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    m_SkyboxVBO = vbo;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kSkyboxVerts), kSkyboxVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    return vao;
}
} // namespace RetroRenderer
