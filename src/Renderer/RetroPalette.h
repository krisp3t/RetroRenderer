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
const Color& GetPaletteColor(Config::PaletteType palette, size_t index);

uint8_t FindNearestPaletteIndex(const Color& color, Config::PaletteType palette);
Color FindNearestPaletteColor(const Color& color, Config::PaletteType palette);
Pixel FindNearestPalettePixel(const Color& color, Config::PaletteType palette);

float GetOrderedDitherThreshold4x4(const glm::ivec2& pixelPos);
Color ApplyOrderedDither4x4(const Color& color,
                            const glm::ivec2& pixelPos,
                            Config::PaletteType palette,
                            float strength = 1.0f);

float QuantizeUnitToBands(float value, int bandCount);
Color SampleRamp(Config::PaletteType palette, uint8_t basePaletteIndex, float value, int bandCount = 4);

} // namespace RetroPalette
} // namespace RetroRenderer
