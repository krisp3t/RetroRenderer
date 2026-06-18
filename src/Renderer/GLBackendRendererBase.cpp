#include "GLBackendRendererBase.h"
#include "../Scene/Model.h"

#include <KrisLogger/Logger.h>
#include <cassert>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace RetroRenderer {
namespace {

const char* ShaderTypeName(GLenum shaderType) {
    switch (shaderType) {
    case GL_VERTEX_SHADER:
        return "vertex";
    case GL_FRAGMENT_SHADER:
        return "fragment";
    default:
        return "unknown";
    }
}

const char* FramebufferStatusName(GLenum status) {
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        return "GL_FRAMEBUFFER_COMPLETE";
    case GL_FRAMEBUFFER_UNDEFINED:
        return "GL_FRAMEBUFFER_UNDEFINED";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
#ifndef __EMSCRIPTEN__
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
#endif
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
#if defined(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
#endif
    default:
        return "GL_FRAMEBUFFER_STATUS_UNKNOWN";
    }
}

GLenum DepthAttachmentStorageFormat() {
#if defined(__EMSCRIPTEN__)
    return GL_DEPTH_COMPONENT16;
#else
    return GL_DEPTH24_STENCIL8;
#endif
}

GLenum DepthAttachmentTarget() {
#if defined(__EMSCRIPTEN__)
    return GL_DEPTH_ATTACHMENT;
#else
    return GL_DEPTH_STENCIL_ATTACHMENT;
#endif
}

} // namespace

bool GLBackendRendererBase::InitializeBackend(TextureHandle fbTex, int w, int h, const std::string& cubemapPath) {
    ConfigureBackendDebugState();
    if (!CreateFramebuffer(fbTex, w, h)) {
        return false;
    }

    CreateFallbackTexture();
    const GLuint cubeTex = CreateCubemap(cubemapPath);
    if (cubeTex != 0) {
        m_SkyboxTexture = cubeTex;
        m_SkyboxProgram = MaterialManager::CreateShaderProgram(*this, "shaders/skybox.vs", "shaders/skybox.fs");
        if (m_SkyboxProgram.handle.IsValid()) {
            m_SkyboxVAO = CreateSkyboxVAO();
        }
    }

    InitializeBackendResources();
    glViewport(0, 0, w, h);
    return true;
}

bool GLBackendRendererBase::CreateFramebuffer(TextureHandle fbTex, int w, int h) {
    DestroyFramebufferResources();
    m_FrameBufferTexture = ToGLHandle(fbTex);
    m_FrameWidth = w;
    m_FrameHeight = h;

    if (m_FrameBufferTexture == 0 || !glIsTexture(m_FrameBufferTexture)) {
        LOGE("%s framebuffer texture handle %u is not a valid GL texture", GetRendererLogLabel(), m_FrameBufferTexture);
        DestroyFramebufferResources();
        return false;
    }

    glGenFramebuffers(1, &m_FrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FrameBufferTexture, 0);
    constexpr GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers);

    glGenRenderbuffers(1, &m_DepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, DepthAttachmentStorageFormat(), w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, DepthAttachmentTarget(), GL_RENDERBUFFER, m_DepthBuffer);

    const GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("%s framebuffer is not complete (status=0x%x, %s)",
             GetRendererLogLabel(),
             framebufferStatus,
             FramebufferStatusName(framebufferStatus));
        DestroyFramebufferResources();
        return false;
    }

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void GLBackendRendererBase::Resize(TextureHandle newFbTex, int w, int h) {
    glViewport(0, 0, w, h);
    if (!CreateFramebuffer(newFbTex, w, h)) {
        LOGE("Failed to resize %s framebuffer to %d x %d", GetRendererLogLabel(), w, h);
    }
}

void GLBackendRendererBase::Destroy() {
    m_MeshResources.Clear();
    m_TextureResources.Clear();
    DestroyFramebufferResources();
    DestroyRendererResources();
    m_UniformCache.Clear();
    m_ActiveCamera = nullptr;
    m_SceneLights.clear();
    m_FrameMaterialState = {};
}

void GLBackendRendererBase::DestroyFramebufferResources() {
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
    m_FrameBufferTexture = 0;
    m_FrameWidth = 0;
    m_FrameHeight = 0;
}

void GLBackendRendererBase::DestroyRendererResources() {
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

    DestroyBackendResources();
}

void GLBackendRendererBase::InvalidateSceneResources() {
    m_MeshResources.Clear();
    m_TextureResources.Clear();
}

void GLBackendRendererBase::InvalidateTextureResources() {
    m_TextureResources.Clear();
}

void GLBackendRendererBase::RenderFrame(const FrameSnapshot& frame) {
    if (!frame.hasScene) {
        return;
    }

    m_FrameCameraSnapshot = frame.camera;
    m_FrameConfigSnapshot = frame.configSnapshot;
    m_FrameMaterialState = frame.materials.empty() ? FrameMaterialState{} : frame.materials.front();
    SetActiveCamera(m_FrameCameraSnapshot);
    SetSceneLights(frame.lights);

    BeforeFrame(frame.clearColor);
    if (frame.configSnapshot.environment.showSkybox) {
        DrawSkybox();
    }

    for (const RenderItem& item : frame.items) {
        if (!item.mesh) {
            continue;
        }

        const FrameMaterialState* materialState = ResolveFrameMaterial(frame, item.materialId);
        if (materialState == nullptr) {
            continue;
        }

        DrawMesh(*item.mesh, item.worldTransform, ResolveFrameTexture(frame, item.textureId), *materialState, frame.configSnapshot);
    }

    RenderBackendOverlays();
    EndFrame();
}

void GLBackendRendererBase::SetActiveCamera(const Camera& camera) {
    m_ActiveCamera = &camera;
}

void GLBackendRendererBase::SetSceneLights(const std::vector<LightSnapshot>& lights) {
    m_SceneLights = lights;
}

void GLBackendRendererBase::DrawTriangularMesh(const Model* model) {
    assert(model != nullptr && "Tried to draw null model");

    for (const Mesh& mesh : model->GetMeshes()) {
        DrawMesh(mesh, model->GetWorldTransform(), mesh.GetPrimaryTexture(), m_FrameMaterialState, m_FrameConfigSnapshot);
    }
}

void GLBackendRendererBase::DrawMesh(const Mesh& mesh,
                                     const glm::mat4& worldTransform,
                                     const Texture* texture,
                                     const FrameMaterialState& materialState,
                                     const Config& configSnapshot) {
    DrawMeshGpuResources(m_MeshResources.GetOrCreate(mesh), worldTransform, texture, materialState, configSnapshot);
}

void GLBackendRendererBase::DrawMesh(const RenderMeshSnapshot& mesh,
                                     const glm::mat4& worldTransform,
                                     const Texture* texture,
                                     const FrameMaterialState& materialState,
                                     const Config& configSnapshot) {
    DrawMeshGpuResources(m_MeshResources.GetOrCreate(mesh), worldTransform, texture, materialState, configSnapshot);
}

void GLBackendRendererBase::DrawMeshGpuResources(const GLMeshResourceCache::MeshGpuResources& gpuMesh,
                                                 const glm::mat4& worldTransform,
                                                 const Texture* texture,
                                                 const FrameMaterialState& materialState,
                                                 const Config& configSnapshot) {
    const GLuint shaderProgram = ToGLHandle(materialState.shaderHandle);
    if (shaderProgram == 0 || m_ActiveCamera == nullptr) {
        return;
    }

    const auto& uniforms = m_UniformCache.GetMeshProgram(shaderProgram);
    glUseProgram(shaderProgram);

    const glm::vec3 lightPos =
        !m_SceneLights.empty() ? m_SceneLights.front().position : configSnapshot.environment.lightPosition;
    const glm::mat4& viewMat = m_ActiveCamera->m_ViewMat;
    const glm::mat4& projMat = m_ActiveCamera->m_ProjMat;
    const glm::mat4 mv = viewMat * worldTransform;
    const glm::mat4 mvp = projMat * mv;
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldTransform)));

    if (uniforms.modelMatrix >= 0) {
        glUniformMatrix4fv(uniforms.modelMatrix, 1, GL_FALSE, glm::value_ptr(worldTransform));
    }
    if (uniforms.mvp >= 0) {
        glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
    }
    if (uniforms.normalMatrix >= 0) {
        glUniformMatrix3fv(uniforms.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    if (uniforms.lightPos >= 0) {
        glUniform3f(uniforms.lightPos, lightPos.x, lightPos.y, lightPos.z);
    }
    if (uniforms.viewPos >= 0) {
        glUniform3f(uniforms.viewPos,
                    m_ActiveCamera->m_Position.x,
                    m_ActiveCamera->m_Position.y,
                    m_ActiveCamera->m_Position.z);
    }
    if (uniforms.lightColor >= 0) {
        glUniform3f(uniforms.lightColor,
                    materialState.lightColor.r,
                    materialState.lightColor.g,
                    materialState.lightColor.b);
    }

    if (materialState.enablePhong) {
        if (uniforms.shininess >= 0) {
            glUniform1f(uniforms.shininess, materialState.shininess);
        }
        if (uniforms.ambientStrength >= 0) {
            glUniform1f(uniforms.ambientStrength, materialState.ambientStrength);
        }
        if (uniforms.specularStrength >= 0) {
            glUniform1f(uniforms.specularStrength, materialState.specularStrength);
        }
    }

    const GLuint gpuTexture = texture != nullptr ? m_TextureResources.GetOrCreate(*texture) : 0;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gpuTexture != 0 ? gpuTexture : m_FallbackTexture);
    if (uniforms.texture >= 0) {
        glUniform1i(uniforms.texture, 0);
    }

    glBindVertexArray(gpuMesh.vao);
    glDrawElements(GL_TRIANGLES, gpuMesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GLBackendRendererBase::BeforeFrame(const Color& clearColor) {
    auto c = clearColor.ToImVec4();

    // Keep the offscreen framebuffer bound for the whole frame to avoid per-draw rebinding.
    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    glViewport(0, 0, m_FrameWidth, m_FrameHeight);
    glClearColor(c.x, c.y, c.z, c.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    ApplyBackendFrameState();
}

void GLBackendRendererBase::EndFrame() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ResetBackendFrameState();
}

bool GLBackendRendererBase::CheckShaderErrors(GLuint object, const char* stage, bool isProgram) const {
    GLint success = GL_TRUE;
    if (isProgram) {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
    } else {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
    }

    if (success == GL_FALSE) {
        GLint logLength = 0;
        if (isProgram) {
            glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
        } else {
            glGetShaderiv(object, GL_INFO_LOG_LENGTH, &logLength);
        }

        std::vector<char> errorLog(static_cast<size_t>(logLength > 0 ? logLength : 1), '\0');
        if (isProgram) {
            glGetProgramInfoLog(object, logLength, &logLength, errorLog.data());
            LOGE("%s: error linking %s: %s", GetRendererLogLabel(), stage, errorLog.data());
        } else {
            glGetShaderInfoLog(object, logLength, &logLength, errorLog.data());
            LOGE("%s: error compiling %s shader: %s", GetRendererLogLabel(), stage, errorLog.data());
        }
        return false;
    }

    return true;
}

GLuint GLBackendRendererBase::CompileShaderStage(GLenum shaderType, const std::string& shaderSource) {
    const std::string_view prefix = GetShaderPrefix();
    std::string fullSource;
    fullSource.reserve(prefix.size() + shaderSource.size());
    fullSource.append(prefix.data(), prefix.size());
    fullSource.append(shaderSource);

    const char* srcPtr = fullSource.c_str();
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        LOGE("%s: failed to create %s shader", GetRendererLogLabel(), ShaderTypeName(shaderType));
        return 0;
    }

    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);
    if (!CheckShaderErrors(shader, ShaderTypeName(shaderType), false)) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

ShaderHandle GLBackendRendererBase::CompileShaders(const std::string& vertexCode, const std::string& fragmentCode) {
    const GLuint vertexShader = CompileShaderStage(GL_VERTEX_SHADER, vertexCode);
    if (vertexShader == 0) {
        return {};
    }

    const GLuint fragmentShader = CompileShaderStage(GL_FRAGMENT_SHADER, fragmentCode);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return {};
    }

    const GLuint shaderProgram = glCreateProgram();
    if (shaderProgram == 0) {
        LOGE("%s: failed to create shader program", GetRendererLogLabel());
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return {};
    }

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    if (!CheckShaderErrors(shaderProgram, "program", true)) {
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

void GLBackendRendererBase::CreateFallbackTexture() {
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

GLuint GLBackendRendererBase::CreateCubemap(const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        LOGW("Could not load cubemap %s", path.c_str());
        return 0;
    }

    const int fullWidth = surface->w;
    const int fullHeight = surface->h;
    const int faceSize = fullWidth / 4;
    if (fullHeight != faceSize * 3) {
        LOGE("Invalid cubemap cross layout dimensions: %dx%d", fullWidth, fullHeight);
        SDL_FreeSurface(surface);
        return 0;
    }

    GLuint cubemapTex = 0;
    glGenTextures(1, &cubemapTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);

    const int bpp = surface->format->BytesPerPixel;
    auto* pixels = static_cast<Uint8*>(surface->pixels);

    struct Offset {
        int x;
        int y;
    };
    constexpr Offset kFaceOffsets[6] = {
        {2, 1},
        {0, 1},
        {1, 0},
        {1, 2},
        {1, 1},
        {3, 1},
    };

    std::vector<Uint8> facePixels(static_cast<size_t>(faceSize * faceSize * bpp));
    for (int face = 0; face < 6; ++face) {
        const int ox = kFaceOffsets[face].x * faceSize;
        const int oy = kFaceOffsets[face].y * faceSize;

        for (int row = 0; row < faceSize; ++row) {
            std::memcpy(facePixels.data() + static_cast<size_t>(row * faceSize * bpp),
                        pixels + static_cast<size_t>((oy + row) * fullWidth * bpp + ox * bpp),
                        static_cast<size_t>(faceSize * bpp));
        }

        const GLenum format = bpp == 4 ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                     0,
                     format,
                     faceSize,
                     faceSize,
                     0,
                     format,
                     GL_UNSIGNED_BYTE,
                     facePixels.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    SDL_FreeSurface(surface);
    return cubemapTex;
}

void GLBackendRendererBase::DrawSkybox() {
    if (m_ActiveCamera == nullptr || m_FrameBuffer == 0 || !m_SkyboxProgram.handle.IsValid() || m_SkyboxVAO == 0 || m_SkyboxTexture == 0) {
        return;
    }

    const GLuint skyboxProgram = ToGLHandle(m_SkyboxProgram.handle);
    const auto& uniforms = m_UniformCache.GetSkyboxProgram(skyboxProgram);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glUseProgram(skyboxProgram);

    const glm::mat4 view = glm::mat4(glm::mat3(m_ActiveCamera->m_ViewMat));
    const glm::mat4 proj = m_ActiveCamera->m_ProjMat;

    if (uniforms.skyboxSampler >= 0) {
        glUniform1i(uniforms.skyboxSampler, 0);
    }
    if (uniforms.viewMatrix >= 0) {
        glUniformMatrix4fv(uniforms.viewMatrix, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (uniforms.projectionMatrix >= 0) {
        glUniformMatrix4fv(uniforms.projectionMatrix, 1, GL_FALSE, glm::value_ptr(proj));
    }

    glBindVertexArray(m_SkyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glUseProgram(0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

GLuint GLBackendRendererBase::CreateSkyboxVAO() {
    if (m_SkyboxVBO != 0) {
        glDeleteBuffers(1, &m_SkyboxVBO);
        m_SkyboxVBO = 0;
    }
    if (m_SkyboxVAO != 0) {
        glDeleteVertexArrays(1, &m_SkyboxVAO);
        m_SkyboxVAO = 0;
    }

    static const float kSkyboxVerts[] = {
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
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
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    m_SkyboxVBO = vbo;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kSkyboxVerts), kSkyboxVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return vao;
}

} // namespace RetroRenderer
