#pragma once

#include "FrameSubmission.h"
#include "GLFramePresenter.h"
#include "GLUiRenderer.h"
#include "IHardwareRenderer.h"
#include "IRenderExecutor.h"
#include "../Base/Stats.h"
#include <SDL.h>
#include <memory>
#include <thread>

namespace RetroRenderer {

class InlineRenderExecutor final : public IRenderExecutor {
  public:
    bool Init(SDL_Window* window,
              SDL_GLContext context,
              const std::shared_ptr<Config>& config,
              const std::shared_ptr<Stats>& stats,
              const UiFontAtlas& fontAtlas) override;
    void Execute(FrameSubmission&& submission) override;
    void Destroy() override;

  private:
    void AssertOwnerThread() const;
    GLuint ResolveUiTexture(TextureHandle handle) const;
    bool ApplyOutputConfiguration(const RenderPacket& packet);
    void ApplyUiTextureSnapshots(const std::vector<UiTextureSnapshot>& snapshots);

    SDL_Window* m_Window = nullptr;
    SDL_GLContext m_Context = nullptr;
    std::shared_ptr<Stats> p_Stats;
    std::unique_ptr<IHardwareRenderer> m_GLRenderer;
    GLFramePresenter m_OutputPresenter;
    GLFramePresenter m_PreviewPresenter;
    GLFramePresenter m_FontPresenter;
    GLUiRenderer m_UiRenderer;
    std::thread::id m_OwnerThread;
    glm::ivec2 m_OutputSize{0, 0};
    bool m_NearestNeighbor = false;
    bool m_Vsync = true;
    bool m_Initialized = false;
};

} // namespace RetroRenderer
