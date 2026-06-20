#pragma once
#include "../Base/Color.h"
#include <SDL_image.h>
#include <array>
#include <glm/vec2.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace RetroRenderer {
class Texture {
  public:
    static constexpr size_t kAutoPaletteSize = 16;

    Texture();
    ~Texture() = default;
    // Disable copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    // Enable moving
    Texture(Texture&& other) noexcept = default;
    Texture& operator=(Texture&& other) noexcept = default;

    bool LoadFromFile(const char* filePath);
    bool LoadFromMemory(const uint8_t* data, const size_t size);
    bool IsValid() const {
        return HasCpuPixels();
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
    uint64_t GetRevision() const {
        return m_Revision;
    }
    const std::vector<Pixel>& GetPixels() const {
        return m_Pixels;
    }
    bool HasAutoPalette() const {
        return m_HasAutoPalette;
    }
    [[nodiscard]] uint64_t EstimateResidentCpuBytes() const;
    Texture CloneCpuOnly() const;
    Pixel SampleNearestRepeat(const glm::vec2& uv) const;
    Pixel SampleReducedNearestRepeat(const glm::vec2& uv, int maxDimension) const;
    uint8_t FindNearestAutoPaletteIndex(const Color& color) const;
    uint8_t FindNearestAutoPaletteIndex(uint8_t r, uint8_t g, uint8_t b) const;
    Pixel FindNearestAutoPalettePixel(const Color& color) const;
    Pixel SampleAutoRampPixel(uint8_t basePaletteIndex, float value, int bandCount = 4, uint8_t alpha = 255) const;
    const std::array<Pixel, 16>& GetAutoDitherPattern4x4(uint8_t paletteIndex) const;
    const std::array<Pixel, kAutoPaletteSize>& GetAutoPalettePixels() const {
        return m_AutoPalette;
    }

  private:
    static constexpr size_t kRgb5LutSize = 32 * 32 * 32;

    bool LoadTextureFromFile(const char* filePath, std::vector<Pixel>& outPixels, int& outWidth, int& outHeight);
    bool LoadTextureFromMemory(const uint8_t* data,
                               const size_t size,
                               std::vector<Pixel>& outPixels,
                               int& outWidth,
                               int& outHeight);
    void ClearAutoPaletteCaches();
    void RebuildAutoPaletteCaches();

    std::string m_Path;
    int m_Width = 0;
    int m_Height = 0;
    uint64_t m_Revision = 0;
    std::vector<Pixel> m_Pixels;
    bool m_HasAutoPalette = false;
    std::array<Pixel, kAutoPaletteSize> m_AutoPalette{};
    std::array<std::array<Pixel, 4>, kAutoPaletteSize> m_AutoRampPixels{};
    std::array<std::array<Pixel, 16>, kAutoPaletteSize> m_AutoDitherPatterns{};
    std::array<uint8_t, kRgb5LutSize> m_AutoPaletteNearestIndexLut{};
};
} // namespace RetroRenderer
