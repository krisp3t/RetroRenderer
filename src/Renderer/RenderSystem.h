#pragma once
#include "../Base/Color.h"
#include "../Base/Event.h"
#include "../Base/Stats.h"
#include "../Scene/Scene.h"
#include "../Window/DisplaySystem.h"
#include "Software/SWRenderer.h"
#if !defined(__EMSCRIPTEN__)
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
#endif
#include <memory>
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#include "GLES/GLESRenderer.h"
#else
#include "OpenGL/GLRenderer.h"
#endif

namespace RetroRenderer {

class RenderSystem {
  public:
    RenderSystem() = default;

    ~RenderSystem();

    static void CreateFramebufferTexture(GLuint& texId, int width, int height);

    bool Init();

    void BeforeFrame(const Color& clearColor);

    [[nodiscard]] std::vector<int>& BuildRenderQueue(Scene& scene, const Camera& camera);

    GLuint Render(const std::shared_ptr<Scene>& scene, const Camera& camera, const std::vector<int>& renderQueue);

    void Resize(const glm::ivec2& resolution);

    void Destroy();

    void OnLoadScene(const SceneLoadEvent& e);

    [[nodiscard]] GLuint CompileShaders(const std::string& vertexCode, const std::string& fragmentCode);

  private:
    struct SoftwareRenderJob {
        std::shared_ptr<Scene> scene = nullptr;
        std::shared_ptr<Camera> camera = nullptr;
        std::vector<int> renderQueue;
        Color clearColor{};
        bool showSkybox = false;
    };

    void StartSoftwareWorker();
    void StopSoftwareWorker();
    void SubmitSoftwareJob(const std::shared_ptr<Scene>& scene, const Camera& camera, const std::vector<int>& renderQueue);
    void UploadSoftwareFrameToTexture();
    void SoftwareWorkerLoop();
    GLuint RenderSoftwareSync(const std::shared_ptr<Scene>& scene, const Camera& camera, const std::vector<int>& renderQueue);

  private:
    std::unique_ptr<Scene> p_scene_ = nullptr;
    std::unique_ptr<SWRenderer> p_SWRenderer_ = nullptr;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    std::unique_ptr<GLESRenderer> p_GLRenderer_ = nullptr;
#else
    std::unique_ptr<GLRenderer> p_GLRenderer_ = nullptr;
#endif
    IRenderer* p_activeRenderer_ = nullptr;

    GLuint m_SWFramebufferTexture = 0;
    GLuint m_GLFramebufferTexture = 0;
    Color m_SoftwareClearColor = Color::DefaultBackground();

#if !defined(__EMSCRIPTEN__)
    std::thread m_SoftwareWorkerThread;
    std::condition_variable m_SoftwareWorkerCv;
    std::mutex m_SoftwareWorkerMutex;
    std::optional<SoftwareRenderJob> m_PendingSoftwareJob;
    std::vector<Pixel> m_CompletedSoftwareFrame;
    size_t m_CompletedSoftwareFrameWidth = 0;
    size_t m_CompletedSoftwareFrameHeight = 0;
    bool m_SoftwareWorkerStopRequested = false;
    bool m_HasCompletedSoftwareFrame = false;
#endif
};

} // namespace RetroRenderer
