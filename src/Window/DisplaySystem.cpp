#include <imgui.h>
#include "DisplaySystem.h"
#include "../Base/Logger.h"
#include "../Renderer/Buffer.h"

namespace RetroRenderer
{
    bool DisplaySystem::Init()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            LOGE("Unable to initialize SDL: %s\n", SDL_GetError());
            return false;
        }
        m_Window = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_ScreenWidth, m_ScreenHeight, kWindowFlags);
        if (m_Window == nullptr)
        {
            LOGE("Unable to create window: %s", SDL_GetError());
            return false;
        }
        m_SDLRenderer = SDL_CreateRenderer(m_Window, -1, kRendererFlags);
        if (m_SDLRenderer == nullptr)
        {
            LOGE("Unable to create renderer: %s", SDL_GetError());
            return false;
        }
       m_ScreenTexture = SDL_CreateTexture(m_SDLRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_ScreenWidth, m_ScreenHeight);
        if (m_ScreenTexture == nullptr)
        {
            LOGE("Unable to create texture: %s", SDL_GetError());
            return false;
        }
        const float scale{GetScale()};
        SDL_RenderSetScale(m_SDLRenderer, scale, scale);

        p_Config = std::make_shared<Config>();
        m_ConfigPanel = std::make_unique<ConfigPanel>(m_Window, m_SDLRenderer, p_Config);
        return true;
    }

    float DisplaySystem::GetScale() const {
        int window_width{0};
        int window_height{0};
        SDL_GetWindowSize(
                m_Window,
                &window_width, &window_height
        );

        int render_output_width{0};
        int render_output_height{0};
        SDL_GetRendererOutputSize(
                m_SDLRenderer,
                &render_output_width, &render_output_height
        );

        const auto scale_x{
                static_cast<float>(render_output_width) /
                static_cast<float>(window_width)
        };

        return scale_x;
    }

    void DisplaySystem::BeforeFrame() {
        ImU32 c = ImGui::ColorConvertFloat4ToU32(p_Config->renderer.clearColor);
        SDL_SetRenderDrawColor(m_SDLRenderer, c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, (c >> 24) & 0xFF);
        SDL_RenderClear(m_SDLRenderer);
        m_ConfigPanel.get()->BeforeFrame(m_SDLRenderer);
    }

    void DisplaySystem::DrawFrame()
    {
        m_ConfigPanel.get()->OnDraw();
    }
    void DisplaySystem::DrawFrame(const Buffer<Uint32> &buffer)
    {
        assert(buffer.width == m_ScreenWidth && buffer.height == m_ScreenHeight && "Buffer size does not match window size");
        assert(buffer.data.get() != nullptr && "Buffer data is null");
        assert(m_ScreenTexture != nullptr && "Screen texture is null");

        const Uint32 *src = buffer.data.get();
        SDL_UpdateTexture(m_ScreenTexture, nullptr, src, static_cast<int>(buffer.pitch));
        SDL_RenderCopy(m_SDLRenderer, m_ScreenTexture, nullptr, nullptr);
        m_ConfigPanel.get()->OnDraw();
    }

    void DisplaySystem::SwapBuffers()
    {
        SDL_RenderPresent(m_SDLRenderer);
    }

    int DisplaySystem::GetWidth() const
    {
        return m_ScreenWidth;
    }
    int DisplaySystem::GetHeight() const
    {
        return m_ScreenHeight;
    }

    void DisplaySystem::Destroy()
    {
        SDL_DestroyTexture(m_ScreenTexture);
        SDL_DestroyRenderer(m_SDLRenderer);
        SDL_DestroyWindow(m_Window);
        SDL_Quit();
    }
}