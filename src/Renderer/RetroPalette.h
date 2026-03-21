#pragma once

#include "../Base/Color.h"
#include "../Base/Config.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>

namespace RetroRenderer {
namespace RetroPalette {

struct PaletteRamp {
    const char* name = "";
    std::array<uint8_t, 4> paletteIndices{};
};

constexpr size_t kPico8PaletteSize = 16;

const std::array<Color, kPico8PaletteSize>& GetPico8Palette();
const PaletteRamp& GetPico8Ramp(uint8_t paletteIndex);
const std::array<Color, kPico8PaletteSize>& GetPaletteColors(const Config::RetroStyleSettings& retro);
const Color& GetPaletteColor(Config::PaletteType palette, size_t index);
const Color& GetPaletteColor(const Config::RetroStyleSettings& retro, size_t index);
const std::array<Pixel, 16>& GetOrderedDitherPattern4x4(Config::PaletteType palette, uint8_t paletteIndex);
const std::array<Pixel, 16>& GetOrderedDitherPattern4x4(const Config::RetroStyleSettings& retro, uint8_t paletteIndex);

uint8_t FindNearestPaletteIndex(const Color& color, Config::PaletteType palette);
uint8_t FindNearestPaletteIndex(uint8_t r, uint8_t g, uint8_t b, Config::PaletteType palette);
uint8_t FindNearestPaletteIndex(const Color& color, const Config::RetroStyleSettings& retro);
uint8_t FindNearestPaletteIndex(uint8_t r, uint8_t g, uint8_t b, const Config::RetroStyleSettings& retro);
Color FindNearestPaletteColor(const Color& color, Config::PaletteType palette);
Color FindNearestPaletteColor(const Color& color, const Config::RetroStyleSettings& retro);
Pixel FindNearestPalettePixel(const Color& color, Config::PaletteType palette);
Pixel FindNearestPalettePixel(const Color& color, const Config::RetroStyleSettings& retro);

float GetOrderedDitherThreshold4x4(const glm::ivec2& pixelPos);
Color ApplyOrderedDither4x4(const Color& color,
                            const glm::ivec2& pixelPos,
                            Config::PaletteType palette,
                            float strength = 1.0f);
Color ApplyOrderedDither4x4(const Color& color,
                            const glm::ivec2& pixelPos,
                            const Config::RetroStyleSettings& retro,
                            float strength = 1.0f);

float QuantizeUnitToBands(float value, int bandCount);
Color SampleRamp(Config::PaletteType palette, uint8_t basePaletteIndex, float value, int bandCount = 4);
Color SampleRamp(const Config::RetroStyleSettings& retro, uint8_t basePaletteIndex, float value, int bandCount = 4);
Pixel SampleRampPixel(Config::PaletteType palette,
                      uint8_t basePaletteIndex,
                      float value,
                      int bandCount = 4,
                      uint8_t alpha = 255);
Pixel SampleRampPixel(const Config::RetroStyleSettings& retro,
                      uint8_t basePaletteIndex,
                      float value,
                      int bandCount = 4,
                      uint8_t alpha = 255);
void CopyPaletteToCustom(Config::RetroStyleSettings& retro, Config::PaletteType sourcePalette);

} // namespace RetroPalette
} // namespace RetroRenderer
