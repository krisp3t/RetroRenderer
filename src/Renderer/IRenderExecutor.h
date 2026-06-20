#pragma once

#include "FrameSubmission.h"
#include "../Base/Stats.h"
#include <SDL.h>
#include <memory>

namespace RetroRenderer {

class IRenderExecutor {
  public:
    virtual ~IRenderExecutor() = default;

    virtual bool Init(SDL_Window* window,
                      SDL_GLContext context,
                      const std::shared_ptr<Config>& config,
                      const std::shared_ptr<Stats>& stats,
                      const UiFontAtlas& fontAtlas) = 0;
    virtual void Execute(FrameSubmission&& submission) = 0;
    virtual void Destroy() = 0;
};

} // namespace RetroRenderer
