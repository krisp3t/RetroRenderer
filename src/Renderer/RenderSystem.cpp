#include "RenderSystem.h"
#include "../Engine.h"
#include <KrisLogger/Logger.h>
#include <cassert>
#include <iostream>

namespace RetroRenderer {
bool RenderSystem::Init(SDL_Window *window) {
    auto const &p_config = Engine::Get().GetConfig();
    p_SWRenderer_ = std::make_unique<SWRenderer>();
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    p_GLRenderer_ = std::make_unique<GLESRenderer>();
#else
    p_GLRenderer_ = std::make_unique<GLRenderer>();
#endif

    p_activeRenderer_ = p_GLRenderer_.get();
    auto &fbResolution = p_config->renderer.resolution;
    assert(fbResolution.x > 0 && fbResolution.y > 0 && "Tried to initialize renderers with invalid resolution");

    CreateFramebufferTexture(m_SWFramebufferTexture, fbResolution.x, fbResolution.y);
    CreateFramebufferTexture(m_GLFramebufferTexture, fbResolution.x, fbResolution.y);

    if (!p_SWRenderer_->Init(fbResolution.x, fbResolution.y)) {
        LOGE("Failed to initialize SWRenderer");
        return false;
    }
    if (!p_GLRenderer_->Init(m_GLFramebufferTexture, fbResolution.x, fbResolution.y)) {
        LOGE("Failed to initialize GLRenderer");
        return false;
    }

    return true;
}

void RenderSystem::CreateFramebufferTexture(GLuint &texId, int width, int height) {
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // TODO: make filtering configurable
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderSystem::BeforeFrame(const Color &clearColor) {
    auto &p_config = Engine::Get().GetConfig();
    switch (p_config->renderer.selectedRenderer) {
    case Config::RendererType::SOFTWARE:
        p_activeRenderer_ = p_SWRenderer_.get();
        break;
    case Config::RendererType::GL:
        p_activeRenderer_ = p_GLRenderer_.get();
        break;
    default:
        LOGE("Renderer type %d not implemented!", p_config->renderer.selectedRenderer);
        return;
    }
    assert(p_activeRenderer_ != nullptr && "Active renderer is null");
    glViewport(0, 0, p_config->renderer.resolution.x, p_config->renderer.resolution.y);
    p_activeRenderer_->BeforeFrame(clearColor);
}

std::vector<int> &RenderSystem::BuildRenderQueue(Scene &scene, const Camera &camera) {
    p_activeRenderer_->SetActiveCamera(camera);
    return scene.GetVisibleModels(); // TODO: split into meshes?
}

GLuint RenderSystem::Render(std::vector<int> &renderQueue, std::vector<Model> &models) {
    auto const &p_config = Engine::Get().GetConfig();
    auto const &p_stats = Engine::Get().GetStats();
    assert(p_stats != nullptr && "Stats not initialized!");
    p_stats->Reset();

    if (p_config->environment.showSkybox) {
        p_activeRenderer_->DrawSkybox();
    }

    // LOGD("Render queue size: %d", renderQueue.size());
    for (int modelIx : renderQueue) {
        const Model *model = &models[modelIx];
        assert(model != nullptr && "Model is null");
        if (!model->GetMeshes().empty()) {
            p_activeRenderer_->DrawTriangularMesh(model);
        }
    }
    if (p_activeRenderer_ == p_SWRenderer_.get()) {
        const auto *buffer = p_SWRenderer_->GetFrameBuffer();
        assert(buffer != nullptr && "SW framebuffer not initialized");
        glBindTexture(GL_TEXTURE_2D, m_SWFramebufferTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(buffer->width),
                        static_cast<GLsizei>(buffer->height), GL_RGBA, GL_UNSIGNED_BYTE, buffer->data);
        glBindTexture(GL_TEXTURE_2D, 0);
        return m_SWFramebufferTexture;
    }
    return p_activeRenderer_->EndFrame();
}

void RenderSystem::Resize(const glm::ivec2 &resolution) {
    assert(resolution.x > 0 && resolution.y > 0 && "Tried to resize renderer with invalid resolution");
    auto const &p_config = Engine::Get().GetConfig();
    p_config->renderer.resolution = resolution;
    CreateFramebufferTexture(m_SWFramebufferTexture, resolution.x, resolution.y);
    CreateFramebufferTexture(m_GLFramebufferTexture, resolution.x, resolution.y);
    LOGI("Resizing output image to %d x %d", resolution.x, resolution.y);
    p_SWRenderer_->Resize(resolution.x, resolution.y);
    p_GLRenderer_->Resize(m_GLFramebufferTexture, resolution.x, resolution.y);
}

void RenderSystem::Destroy() {
}

void RenderSystem::OnLoadScene(const SceneLoadEvent &e) {
}

GLuint RenderSystem::CompileShaders(const std::string &vertexCode, const std::string &fragmentCode) {
    return p_activeRenderer_->CompileShaders(vertexCode, fragmentCode);
}
} // namespace RetroRenderer
