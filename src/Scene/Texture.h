#pragma once
#include "../Base/Color.h"
#include "../include/kris_glheaders.h"
#include <SDL_image.h>
#include <array>
#include <glm/vec2.hpp>
#include <string>
#include <vector>

namespace RetroRenderer {
class Texture {
  public:
    static constexpr size_t kAutoPaletteSize = 16;

    Texture();
    ~Texture();
    // Disable copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    // Enable moving
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool LoadFromFile(const char* filePath);
    bool LoadFromMemory(const uint8_t* data, const size_t size);
    void Bind(GLuint unit = 0) const;
    GLuint GetID() const {
        return m_TextureID;
    }
    bool IsValid() const {
        return m_TextureID != 0;
    }
    bool HasCpuPixels() const {
        return !m_Pixels.empty() && m_Width > 0 && m_Height > 0;
    }
    const std::string& GetPath() const {
        return m_Path;
    }
    int GetWidth() const {
        return m_Width;
    }
    int GetHeight() const {
        return m_Height;
    }
    bool HasAutoPalette() const {
        return m_HasAutoPalette;
    }
    Texture CloneCpuOnly() const;
    Pixel SampleNearestRepeat(const glm::vec2& uv) const;
    Pixel SampleReducedNearestRepeat(const glm::vec2& uv, int maxDimension) const;
    uint8_t FindNearestAutoPaletteIndex(const Color& color) const;
    uint8_t FindNearestAutoPaletteIndex(uint8_t r, uint8_t g, uint8_t b) const;
    Pixel FindNearestAutoPalettePixel(const Color& color) const;
    Pixel SampleAutoRampPixel(uint8_t basePaletteIndex, float value, int bandCount = 4, uint8_t alpha = 255) const;
    const std::array<Pixel, 16>& GetAutoDitherPattern4x4(uint8_t paletteIndex) const;

  private:
    static constexpr size_t kRgb5LutSize = 32 * 32 * 32;

    bool LoadTextureFromFile(const char* filePath, GLuint& outTextureID, std::vector<Pixel>& outPixels, int& outWidth, int& outHeight);
    bool LoadTextureFromMemory(const uint8_t* data,
                               const size_t size,
                               GLuint& outTextureID,
                               std::vector<Pixel>& outPixels,
                               int& outWidth,
                               int& outHeight);
    void ClearAutoPaletteCaches();
    void RebuildAutoPaletteCaches();

    GLuint m_TextureID;
    std::string m_Path;
    int m_Width = 0;
    int m_Height = 0;
    std::vector<Pixel> m_Pixels;
    bool m_HasAutoPalette = false;
    std::array<Pixel, kAutoPaletteSize> m_AutoPalette{};
    std::array<std::array<Pixel, 4>, kAutoPaletteSize> m_AutoRampPixels{};
    std::array<std::array<Pixel, 16>, kAutoPaletteSize> m_AutoDitherPatterns{};
    std::array<uint8_t, kRgb5LutSize> m_AutoPaletteNearestIndexLut{};
};
} // namespace RetroRenderer
