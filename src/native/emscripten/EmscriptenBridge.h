#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <vector>
#include <KrisLogger/Logger.h>
#include "../../Engine.h"
#include "../../Base/Event.h"

extern "C" EMSCRIPTEN_KEEPALIVE void OnWebFileSelected(uint8_t* data, int size)
{
    LOGD("OnWebFileSelected");
    if (!data || size <= 0) {
        return;
    }

    std::vector<uint8_t> buffer(size);
    std::memcpy(buffer.data(), data, size);

    free(data);

    auto event = std::make_unique<RetroRenderer::SceneLoadEvent>(
        std::move(buffer),
        static_cast<size_t>(size)
    );

    RetroRenderer::Engine::Get().EnqueueEvent(std::move(event));
}
#endif
