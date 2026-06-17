#include "GLESRenderer.h"
#include "../../Base/Debug.h"
#include "../../Engine.h"
#include <KrisLogger/Logger.h>
#include <glm/gtc/type_ptr.hpp>

namespace RetroRenderer {
namespace {
GLuint ToGLHandle(ShaderHandle handle) {
    return static_cast<GLuint>(handle.value);
}

GLuint ToGLHandle(TextureHandle handle) {
    return static_cast<GLuint>(handle.value);
}

FrameMaterialState CaptureMaterialState(const MaterialManager::Material& currentMaterial) {
    FrameMaterialState state{};
    state.shaderHandle = currentMaterial.shaderProgram.handle;
    state.lightColor = currentMaterial.lightColor;
    state.useVertexColor = currentMaterial.name == "phong-vc" ||
                           currentMaterial.shaderProgram.vertexPath.find("phong-vc") != std::string::npos ||
                           currentMaterial.shaderProgram.fragmentPath.find("phong-vc") != std::string::npos;
    if (currentMaterial.phongParams.has_value()) {
        state.enablePhong = true;
        state.ambientStrength = currentMaterial.phongParams->ambientStrength;
        state.specularStrength = currentMaterial.phongParams->specularStrength;
        state.shininess = currentMaterial.phongParams->shininess;
    }
    if (currentMaterial.texture.has_value() && currentMaterial.texture->HasCpuPixels()) {
        state.textureOverride = &*currentMaterial.texture;
    }
    return state;
}
} // namespace

bool GLESRenderer::Init(TextureHandle fbTex, int w, int h) {
    if (!CreateFramebuffer(fbTex, w, h)) {
        return false;
    }

    CreateFallbackTexture();
#ifdef __ANDROID__
    GLuint cubeTex = CreateCubemap("img/skybox-cubemap/Cubemap_Sky_23-512x512.png");
#else
    GLuint cubeTex = CreateCubemap("assets/img/skybox-cubemap/Cubemap_Sky_23-512x512.png");
#endif
    if (cubeTex != 0) {
        m_SkyboxTexture = cubeTex;
        auto skyboxShader = MaterialManager::CreateShaderProgram(*this, "shaders/skybox.vs", "shaders/skybox.fs");
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
bool GLESRenderer::CreateFramebuffer(TextureHandle fbTex, int w, int h) {
    DestroyFramebufferResources();
    p_FrameBufferTexture = ToGLHandle(fbTex);
    m_FrameWidth = w;
    m_FrameHeight = h;

    // Create framebuffer
    glGenFramebuffers(1, &m_FrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_FrameBufferTexture, 0);

    // Create depth+stencil buffer
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

void GLESRenderer::Resize(TextureHandle newFbTex, int w, int h) {
    glViewport(0, 0, w, h);
    if (!CreateFramebuffer(newFbTex, w, h)) {
        LOGE("Failed to resize GLES framebuffer to %d x %d", w, h);
    }
}

void GLESRenderer::Destroy() {
    m_MeshResources.Clear();
    m_TextureResources.Clear();
    DestroyFramebufferResources();
    DestroyRendererResources();
}

void GLESRenderer::DestroyFramebufferResources() {
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

void GLESRenderer::DestroyRendererResources() {
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
}

void GLESRenderer::InvalidateSceneResources() {
    m_MeshResources.Clear();
    m_TextureResources.Clear();
}

void GLESRenderer::InvalidateTextureResources() {
    m_TextureResources.Clear();
}

void GLESRenderer::RenderFrame(const FrameSnapshot& frame) {
    if (!frame.scene) {
        return;
    }

    m_FrameCameraSnapshot = frame.camera;
    m_FrameConfigSnapshot = frame.configSnapshot;
    SetActiveCamera(m_FrameCameraSnapshot);
    SetSceneLights(frame.lights);

    BeforeFrame(frame.clearColor);
    if (frame.configSnapshot.environment.showSkybox) {
        DrawSkybox();
    }

    for (const RenderItem& item : frame.items) {
        if (item.modelIndex < 0 || item.meshIndex < 0) {
            continue;
        }
        if (static_cast<size_t>(item.modelIndex) >= frame.scene->GetModelCount()) {
            continue;
        }

        const Model& model = frame.scene->GetModel(static_cast<size_t>(item.modelIndex));
        if (static_cast<size_t>(item.meshIndex) >= model.GetMeshCount()) {
            continue;
        }

        const Mesh& mesh = model.GetMesh(static_cast<size_t>(item.meshIndex));
        const Texture* texture = frame.materialState.textureOverride != nullptr
                                     ? frame.materialState.textureOverride
                                     : mesh.GetPrimaryTexture();
        DrawMesh(mesh, item.worldTransform, texture, frame.materialState, frame.configSnapshot);
    }

    EndFrame();
}

void GLESRenderer::SetActiveCamera(const Camera& camera) {
    p_Camera = const_cast<Camera*>(&camera);
}

void GLESRenderer::SetSceneLights(const std::vector<LightSnapshot>& lights) {
    m_SceneLights = lights;
}

void GLESRenderer::DrawTriangularMesh(const Model* model) {
    assert(model != nullptr && "Tried to draw null model");

    const MaterialManager::Material& currentMaterial = Engine::Get().GetMaterialManager().GetCurrentMaterial();
    const FrameMaterialState materialState = CaptureMaterialState(currentMaterial);
    const Config& configSnapshot = *Engine::Get().GetConfig();
    for (const Mesh& mesh : model->GetMeshes()) {
        const Texture* texture =
            materialState.textureOverride != nullptr ? materialState.textureOverride : mesh.GetPrimaryTexture();
        DrawMesh(mesh, model->GetWorldTransform(), texture, materialState, configSnapshot);
    }
}

void GLESRenderer::DrawMesh(const Mesh& mesh,
                            const glm::mat4& worldTransform,
                            const Texture* texture,
                            const FrameMaterialState& materialState,
                            const Config& configSnapshot) {
    const GLuint shaderProgram = ToGLHandle(materialState.shaderHandle);
    if (shaderProgram == 0 || p_Camera == nullptr) {
        return;
    }

    glUseProgram(shaderProgram);

    const glm::vec3 lightPos =
        !m_SceneLights.empty() ? m_SceneLights.front().position : configSnapshot.environment.lightPosition;
    const glm::mat4& viewMat = p_Camera->m_ViewMat;
    const glm::mat4& projMat = p_Camera->m_ProjMat;
    const glm::mat4 mv = viewMat * worldTransform;
    const glm::mat4 mvp = projMat * mv;
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldTransform)));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_ModelMatrix"), 1, GL_FALSE, glm::value_ptr(worldTransform));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "u_NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    glUniform3f(glGetUniformLocation(shaderProgram, "u_LightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(
        glGetUniformLocation(shaderProgram, "u_ViewPos"), p_Camera->m_Position.x, p_Camera->m_Position.y, p_Camera->m_Position.z);
    glUniform3f(
        glGetUniformLocation(shaderProgram, "u_LightColor"), materialState.lightColor.r, materialState.lightColor.g, materialState.lightColor.b);

    if (materialState.enablePhong) {
        glUniform1f(glGetUniformLocation(shaderProgram, "u_Shininess"), materialState.shininess);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_AmbientStrength"), materialState.ambientStrength);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_SpecularStrength"), materialState.specularStrength);
    }

    const GLuint gpuTexture = texture != nullptr ? m_TextureResources.GetOrCreate(*texture) : 0;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gpuTexture != 0 ? gpuTexture : m_FallbackTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_Texture"), 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    const auto& gpuMesh = m_MeshResources.GetOrCreate(mesh);
    glBindVertexArray(gpuMesh.vao);
    glDrawElements(GL_TRIANGLES, gpuMesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void GLESRenderer::BeforeFrame(const Color& clearColor) {
    auto c = clearColor.ToImVec4();
    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    glViewport(0, 0, m_FrameWidth, m_FrameHeight);
    glClearColor(c.x, c.y, c.z, c.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLESRenderer::EndFrame() {
}

void GLESRenderer::CheckShaderErrors(GLuint shader, const std::string& type) {
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

GLuint GLESRenderer::CompileShader(GLenum shaderType, const char* shaderSource) {
    constexpr std::string_view kShaderPrefix = "#version 300 es\nprecision highp float;\n";
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
    LOGD("GLESRenderer: Compiled shader: %s", srcPtr);
    return shader;
}

ShaderHandle GLESRenderer::CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) {
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

void GLESRenderer::CreateFallbackTexture() {
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
GLuint GLESRenderer::CreateCubemap(const std::string& path) {
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

void GLESRenderer::DrawSkybox() {
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

GLuint GLESRenderer::CreateSkyboxVAO() {
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
