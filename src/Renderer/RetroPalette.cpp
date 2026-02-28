#include "RetroPalette.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace RetroRenderer {
namespace RetroPalette {
namespace {
constexpr size_t kRgb5AxisSize = 32;
constexpr size_t kRgb5LutSize = kRgb5AxisSize * kRgb5AxisSize * kRgb5AxisSize;

Color MakeColor(uint8_t r, uint8_t g, uint8_t b) {
    return Color(Color::Uint8Tag{}, r, g, b);
}

Color PreserveAlpha(const Color& source, const Color& paletteColor) {
    return Color(Color::Uint8Tag{}, paletteColor.r, paletteColor.g, paletteColor.b, source.a);
}

const std::array<Color, kPico8PaletteSize> kPico8Palette = {
    MakeColor(0x00, 0x00, 0x00),
    MakeColor(0x1D, 0x2B, 0x53),
    MakeColor(0x7E, 0x25, 0x53),
    MakeColor(0x00, 0x87, 0x51),
    MakeColor(0xAB, 0x52, 0x36),
    MakeColor(0x5F, 0x57, 0x4F),
    MakeColor(0xC2, 0xC3, 0xC7),
    MakeColor(0xFF, 0xF1, 0xE8),
    MakeColor(0xFF, 0x00, 0x4D),
    MakeColor(0xFF, 0xA3, 0x00),
    MakeColor(0xFF, 0xEC, 0x27),
    MakeColor(0x00, 0xE4, 0x36),
    MakeColor(0x29, 0xAD, 0xFF),
    MakeColor(0x83, 0x76, 0x9C),
    MakeColor(0xFF, 0x77, 0xA8),
    MakeColor(0xFF, 0xCC, 0xAA),
};

const std::array<PaletteRamp, kPico8PaletteSize> kPico8Ramps = {{
    {"Black", {0, 5, 13, 6}},
    {"Navy", {0, 1, 13, 12}},
    {"Purple", {0, 2, 13, 14}},
    {"Green", {0, 3, 11, 10}},
    {"Brown", {0, 4, 9, 15}},
    {"Slate", {0, 5, 13, 6}},
    {"Gray", {0, 5, 6, 7}},
    {"White", {0, 6, 7, 7}},
    {"Red", {0, 2, 8, 14}},
    {"Orange", {0, 4, 9, 10}},
    {"Yellow", {0, 9, 10, 7}},
    {"Lime", {0, 3, 11, 10}},
    {"Sky", {1, 13, 12, 7}},
    {"Lavender", {1, 13, 6, 7}},
    {"Pink", {2, 8, 14, 7}},
    {"Peach", {4, 9, 15, 7}},
}};

constexpr std::array<uint8_t, 16> kBayer4x4 = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 5,
};

int WeightedColorDistanceSq(const Color& a, const Color& b) {
    const int dr = static_cast<int>(a.r) - static_cast<int>(b.r);
    const int dg = static_cast<int>(a.g) - static_cast<int>(b.g);
    const int db = static_cast<int>(a.b) - static_cast<int>(b.b);
    return dr * dr * 3 + dg * dg * 4 + db * db * 2;
}

Config::PaletteType NormalizePalette(Config::PaletteType palette) {
    return palette == Config::PaletteType::NONE ? Config::PaletteType::PICO8 : palette;
}

size_t QuantizedRgb5Index(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<size_t>(r >> 3) << 10) |
           (static_cast<size_t>(g >> 3) << 5) |
           static_cast<size_t>(b >> 3);
}

uint8_t FindNearestPico8IndexSlow(uint8_t r, uint8_t g, uint8_t b) {
    const Color color(Color::Uint8Tag{}, r, g, b);
    int bestDistance = WeightedColorDistanceSq(color, kPico8Palette[0]);
    uint8_t bestIndex = 0;
    for (size_t i = 1; i < kPico8Palette.size(); i++) {
        const int distance = WeightedColorDistanceSq(color, kPico8Palette[i]);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<uint8_t>(i);
        }
    }
    return bestIndex;
}

const std::array<uint8_t, kRgb5LutSize>& GetPico8NearestIndexLut() {
    static const std::array<uint8_t, kRgb5LutSize> lut = [] {
        std::array<uint8_t, kRgb5LutSize> values{};
        for (size_t r = 0; r < kRgb5AxisSize; r++) {
            for (size_t g = 0; g < kRgb5AxisSize; g++) {
                for (size_t b = 0; b < kRgb5AxisSize; b++) {
                    const uint8_t red = static_cast<uint8_t>((r << 3) | (r >> 2));
                    const uint8_t green = static_cast<uint8_t>((g << 3) | (g >> 2));
                    const uint8_t blue = static_cast<uint8_t>((b << 3) | (b >> 2));
                    values[(r << 10) | (g << 5) | b] = FindNearestPico8IndexSlow(red, green, blue);
                }
            }
        }
        return values;
    }();
    return lut;
}

const std::array<std::array<Pixel, 16>, kPico8PaletteSize>& GetPico8OrderedDitherPatterns() {
    static const std::array<std::array<Pixel, 16>, kPico8PaletteSize> patterns = [] {
        std::array<std::array<Pixel, 16>, kPico8PaletteSize> values{};
        for (size_t paletteIndex = 0; paletteIndex < kPico8PaletteSize; paletteIndex++) {
            const Color& baseColor = kPico8Palette[paletteIndex];
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    const float centeredThreshold = GetOrderedDitherThreshold4x4(glm::ivec2{x, y}) - 0.5f;
                    const int offset = static_cast<int>(std::lround(centeredThreshold * 64.0f));
                    const auto applyOffset = [offset](uint8_t channel) {
                        return static_cast<uint8_t>(std::clamp(static_cast<int>(channel) + offset, 0, 255));
                    };
                    const uint8_t quantizedIndex =
                        FindNearestPico8IndexSlow(applyOffset(baseColor.r), applyOffset(baseColor.g), applyOffset(baseColor.b));
                    const Color& quantizedColor = kPico8Palette[quantizedIndex];
                    values[paletteIndex][static_cast<size_t>((y << 2) | x)] = quantizedColor.ToPixel();
                }
            }
        }
        return values;
    }();
    return patterns;
}

} // namespace

const std::array<Color, kPico8PaletteSize>& GetPico8Palette() {
    return kPico8Palette;
}

const PaletteRamp& GetPico8Ramp(uint8_t paletteIndex) {
    return kPico8Ramps[std::min<size_t>(paletteIndex, kPico8Ramps.size() - 1)];
}

const Color& GetPaletteColor(Config::PaletteType palette, size_t index) {
    switch (NormalizePalette(palette)) {
    case Config::PaletteType::PICO8:
        return kPico8Palette[std::min(index, kPico8Palette.size() - 1)];
    case Config::PaletteType::NONE:
        break;
    }
    return kPico8Palette[0];
}

const std::array<Pixel, 16>& GetOrderedDitherPattern4x4(Config::PaletteType palette, uint8_t paletteIndex) {
    switch (NormalizePalette(palette)) {
    case Config::PaletteType::PICO8:
        return GetPico8OrderedDitherPatterns()[std::min<size_t>(paletteIndex, kPico8PaletteSize - 1)];
    case Config::PaletteType::NONE:
        break;
    }

    return GetPico8OrderedDitherPatterns()[0];
}

uint8_t FindNearestPaletteIndex(const Color& color, Config::PaletteType palette) {
    if (palette == Config::PaletteType::NONE) {
        return 0;
    }

    return FindNearestPaletteIndex(color.r, color.g, color.b, palette);
}

uint8_t FindNearestPaletteIndex(uint8_t r, uint8_t g, uint8_t b, Config::PaletteType palette) {
    switch (NormalizePalette(palette)) {
    case Config::PaletteType::PICO8:
        return GetPico8NearestIndexLut()[QuantizedRgb5Index(r, g, b)];
    case Config::PaletteType::NONE:
        break;
    }
    return 0;
}

Color FindNearestPaletteColor(const Color& color, Config::PaletteType palette) {
    if (palette == Config::PaletteType::NONE) {
        return color;
    }
    const Color& quantized = GetPaletteColor(palette, FindNearestPaletteIndex(color.r, color.g, color.b, palette));
    return PreserveAlpha(color, quantized);
}

Pixel FindNearestPalettePixel(const Color& color, Config::PaletteType palette) {
    if (palette == Config::PaletteType::NONE) {
        return color.ToPixel();
    }
    const Color& quantized = GetPaletteColor(palette, FindNearestPaletteIndex(color.r, color.g, color.b, palette));
    return Pixel{quantized.r, quantized.g, quantized.b, color.a};
}

float GetOrderedDitherThreshold4x4(const glm::ivec2& pixelPos) {
    const int x = ((pixelPos.x % 4) + 4) % 4;
    const int y = ((pixelPos.y % 4) + 4) % 4;
    const int matrixValue = kBayer4x4[static_cast<size_t>(y * 4 + x)];
    return (static_cast<float>(matrixValue) + 0.5f) / 16.0f;
}

Color ApplyOrderedDither4x4(const Color& color,
                            const glm::ivec2& pixelPos,
                            Config::PaletteType palette,
                            float strength) {
    if (palette == Config::PaletteType::NONE) {
        return color;
    }

    if (NormalizePalette(palette) == Config::PaletteType::PICO8 && strength == 1.0f) {
        const uint8_t paletteIndex = FindNearestPaletteIndex(color.r, color.g, color.b, palette);
        const Pixel pixel = GetOrderedDitherPattern4x4(palette, paletteIndex)[static_cast<size_t>(((pixelPos.y & 3) << 2) | (pixelPos.x & 3))];
        return Color(Color::Uint8Tag{}, pixel.r, pixel.g, pixel.b, color.a);
    }

    const float clampedStrength = std::clamp(strength, 0.0f, 2.0f);
    const float centeredThreshold = GetOrderedDitherThreshold4x4(pixelPos) - 0.5f;
    const int offset = static_cast<int>(std::lround(centeredThreshold * clampedStrength * 64.0f));

    auto applyOffset = [offset](uint8_t channel) {
        return static_cast<uint8_t>(std::clamp(static_cast<int>(channel) + offset, 0, 255));
    };

    const Color adjusted(
        Color::Uint8Tag{},
        applyOffset(color.r),
        applyOffset(color.g),
        applyOffset(color.b),
        color.a);
    return FindNearestPaletteColor(adjusted, palette);
}

float QuantizeUnitToBands(float value, int bandCount) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    if (bandCount <= 0) {
        return clamped;
    }
    if (bandCount == 1) {
        return clamped >= 0.5f ? 1.0f : 0.0f;
    }

    const int safeBandCount = std::max(bandCount, 2);
    return std::round(clamped * static_cast<float>(safeBandCount - 1)) / static_cast<float>(safeBandCount - 1);
}

Color SampleRamp(Config::PaletteType palette, uint8_t basePaletteIndex, float value, int bandCount) {
    const Config::PaletteType safePalette = NormalizePalette(palette);
    const PaletteRamp& ramp = GetPico8Ramp(basePaletteIndex);
    const float quantized = QuantizeUnitToBands(value, bandCount);
    const size_t slot =
        std::min<size_t>(static_cast<size_t>(std::lround(quantized * static_cast<float>(ramp.paletteIndices.size() - 1))),
                         ramp.paletteIndices.size() - 1);
    const Color& rampColor = GetPaletteColor(safePalette, ramp.paletteIndices[slot]);
    return rampColor;
}

Pixel SampleRampPixel(Config::PaletteType palette,
                      uint8_t basePaletteIndex,
                      float value,
                      int bandCount,
                      uint8_t alpha) {
    const Config::PaletteType safePalette = NormalizePalette(palette);
    const PaletteRamp& ramp = GetPico8Ramp(basePaletteIndex);
    const float quantized = QuantizeUnitToBands(value, bandCount);
    const size_t slot =
        std::min<size_t>(static_cast<size_t>(std::lround(quantized * static_cast<float>(ramp.paletteIndices.size() - 1))),
                         ramp.paletteIndices.size() - 1);
    const Color& rampColor = GetPaletteColor(safePalette, ramp.paletteIndices[slot]);
    return Pixel{rampColor.r, rampColor.g, rampColor.b, alpha};
}

} // namespace RetroPalette
} // namespace RetroRenderer
