#include "RetroPalette.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace RetroRenderer {
namespace RetroPalette {
namespace {
constexpr size_t kRgb5AxisSize = 32;
constexpr size_t kRgb5LutSize = kRgb5AxisSize * kRgb5AxisSize * kRgb5AxisSize;

struct PaletteData {
    std::array<Color, kPico8PaletteSize> colors{};
    std::array<PaletteRamp, kPico8PaletteSize> ramps{};
    std::array<std::array<Pixel, 16>, kPico8PaletteSize> ditherPatterns{};
    std::array<uint8_t, kRgb5LutSize> nearestIndexLut{};
};

Color MakeColor(uint8_t r, uint8_t g, uint8_t b) {
    return Color(Color::Uint8Tag{}, r, g, b);
}

Color MakeColor(uint32_t rgbHex) {
    return MakeColor(
        static_cast<uint8_t>((rgbHex >> 16) & 0xFF),
        static_cast<uint8_t>((rgbHex >> 8) & 0xFF),
        static_cast<uint8_t>(rgbHex & 0xFF));
}

Color MakeColor(const Pixel& pixel) {
    return Color(Color::Uint8Tag{}, pixel.r, pixel.g, pixel.b, pixel.a);
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

const std::array<Color, kPico8PaletteSize> kDb16Palette = {
    MakeColor(0x14, 0x0C, 0x1C),
    MakeColor(0x44, 0x24, 0x34),
    MakeColor(0x30, 0x34, 0x6D),
    MakeColor(0x4E, 0x4A, 0x4E),
    MakeColor(0x85, 0x4C, 0x30),
    MakeColor(0x34, 0x65, 0x24),
    MakeColor(0xD0, 0x46, 0x48),
    MakeColor(0x75, 0x71, 0x61),
    MakeColor(0x59, 0x7D, 0xCE),
    MakeColor(0xD2, 0x7D, 0x2C),
    MakeColor(0x85, 0x95, 0xA1),
    MakeColor(0x6D, 0xAA, 0x2C),
    MakeColor(0xD2, 0xAA, 0x99),
    MakeColor(0x6D, 0xC2, 0xCA),
    MakeColor(0xDA, 0xD4, 0x5E),
    MakeColor(0xDE, 0xEE, 0xD6),
};

const std::array<Color, kPico8PaletteSize> kSweetie16Palette = {
    MakeColor(0x1A, 0x1C, 0x2C),
    MakeColor(0x5D, 0x27, 0x5D),
    MakeColor(0xB1, 0x3E, 0x53),
    MakeColor(0xEF, 0x7D, 0x57),
    MakeColor(0xFF, 0xCD, 0x75),
    MakeColor(0xA7, 0xF0, 0x70),
    MakeColor(0x38, 0xB7, 0x64),
    MakeColor(0x25, 0x71, 0x79),
    MakeColor(0x29, 0x36, 0x6F),
    MakeColor(0x3B, 0x5D, 0xC9),
    MakeColor(0x41, 0xA6, 0xF6),
    MakeColor(0x73, 0xEF, 0xF7),
    MakeColor(0xF4, 0xF4, 0xF4),
    MakeColor(0x94, 0xB0, 0xC2),
    MakeColor(0x56, 0x6C, 0x86),
    MakeColor(0x33, 0x3C, 0x57),
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

float ColorLuminance(const Color& color) {
    return (0.2126f * static_cast<float>(color.r) +
            0.7152f * static_cast<float>(color.g) +
            0.0722f * static_cast<float>(color.b)) /
           255.0f;
}

Config::PaletteType NormalizeBuiltInPalette(Config::PaletteType palette) {
    if (palette == Config::PaletteType::NONE || palette == Config::PaletteType::CUSTOM) {
        return Config::PaletteType::PICO8;
    }
    return palette;
}

size_t QuantizedRgb5Index(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<size_t>(r >> 3) << 10) |
           (static_cast<size_t>(g >> 3) << 5) |
           static_cast<size_t>(b >> 3);
}

uint8_t FindNearestIndexSlow(const std::array<Color, kPico8PaletteSize>& palette, uint8_t r, uint8_t g, uint8_t b) {
    const Color color(Color::Uint8Tag{}, r, g, b);
    int bestDistance = WeightedColorDistanceSq(color, palette[0]);
    uint8_t bestIndex = 0;
    for (size_t i = 1; i < palette.size(); i++) {
        const int distance = WeightedColorDistanceSq(color, palette[i]);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<uint8_t>(i);
        }
    }
    return bestIndex;
}

std::array<PaletteRamp, kPico8PaletteSize> BuildGenericRamps(const std::array<Color, kPico8PaletteSize>& colors) {
    std::array<PaletteRamp, kPico8PaletteSize> ramps{};
    std::array<size_t, kPico8PaletteSize> luminanceOrder{};
    for (size_t i = 0; i < luminanceOrder.size(); i++) {
        luminanceOrder[i] = i;
    }
    std::sort(luminanceOrder.begin(), luminanceOrder.end(), [&colors](size_t a, size_t b) {
        return ColorLuminance(colors[a]) < ColorLuminance(colors[b]);
    });

    std::array<size_t, kPico8PaletteSize> luminanceRank{};
    for (size_t i = 0; i < luminanceOrder.size(); i++) {
        luminanceRank[luminanceOrder[i]] = i;
    }

    for (size_t paletteIndex = 0; paletteIndex < colors.size(); paletteIndex++) {
        const Color& baseColor = colors[paletteIndex];
        const size_t baseRank = luminanceRank[paletteIndex];
        size_t darkerNearIndex = paletteIndex;
        size_t darkerFarIndex = paletteIndex;
        size_t lighterNearIndex = paletteIndex;
        int darkerNearScore = std::numeric_limits<int>::max();
        int darkerFarScore = std::numeric_limits<int>::max();
        int lighterNearScore = std::numeric_limits<int>::max();

        for (size_t rank = 0; rank < luminanceOrder.size(); rank++) {
            const size_t candidateIndex = luminanceOrder[rank];
            if (candidateIndex == paletteIndex) {
                continue;
            }

            const int rankDistance = std::abs(static_cast<int>(rank) - static_cast<int>(baseRank));
            const int score = WeightedColorDistanceSq(baseColor, colors[candidateIndex]) + rankDistance * 96;
            if (rank < baseRank) {
                if (score < darkerNearScore) {
                    darkerFarIndex = darkerNearIndex;
                    darkerFarScore = darkerNearScore;
                    darkerNearIndex = candidateIndex;
                    darkerNearScore = score;
                } else if (candidateIndex != darkerNearIndex && score < darkerFarScore) {
                    darkerFarIndex = candidateIndex;
                    darkerFarScore = score;
                }
            } else if (rank > baseRank && score < lighterNearScore) {
                lighterNearIndex = candidateIndex;
                lighterNearScore = score;
            }
        }

        if (darkerNearScore == std::numeric_limits<int>::max()) {
            darkerNearIndex = paletteIndex;
        }
        if (darkerFarScore == std::numeric_limits<int>::max()) {
            darkerFarIndex = darkerNearIndex;
        }
        if (lighterNearScore == std::numeric_limits<int>::max()) {
            lighterNearIndex = paletteIndex;
        }

        ramps[paletteIndex] = PaletteRamp{
            "",
            {
                static_cast<uint8_t>(darkerFarIndex),
                static_cast<uint8_t>(darkerNearIndex),
                static_cast<uint8_t>(paletteIndex),
                static_cast<uint8_t>(lighterNearIndex),
            }};
    }

    return ramps;
}

PaletteData BuildPaletteData(const std::array<Color, kPico8PaletteSize>& colors,
                             const std::array<PaletteRamp, kPico8PaletteSize>* rampsOverride = nullptr) {
    PaletteData data{};
    data.colors = colors;
    data.ramps = rampsOverride ? *rampsOverride : BuildGenericRamps(colors);

    for (size_t r = 0; r < kRgb5AxisSize; r++) {
        for (size_t g = 0; g < kRgb5AxisSize; g++) {
            for (size_t b = 0; b < kRgb5AxisSize; b++) {
                const uint8_t red = static_cast<uint8_t>((r << 3) | (r >> 2));
                const uint8_t green = static_cast<uint8_t>((g << 3) | (g >> 2));
                const uint8_t blue = static_cast<uint8_t>((b << 3) | (b >> 2));
                data.nearestIndexLut[(r << 10) | (g << 5) | b] = FindNearestIndexSlow(colors, red, green, blue);
            }
        }
    }

    for (size_t paletteIndex = 0; paletteIndex < colors.size(); paletteIndex++) {
        const Color& baseColor = colors[paletteIndex];
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                const float centeredThreshold = GetOrderedDitherThreshold4x4(glm::ivec2{x, y}) - 0.5f;
                const int offset = static_cast<int>(std::lround(centeredThreshold * 64.0f));
                const auto applyOffset = [offset](uint8_t channel) {
                    return static_cast<uint8_t>(std::clamp(static_cast<int>(channel) + offset, 0, 255));
                };
                const uint8_t quantizedIndex = FindNearestIndexSlow(
                    colors,
                    applyOffset(baseColor.r),
                    applyOffset(baseColor.g),
                    applyOffset(baseColor.b));
                const Color& quantizedColor = colors[quantizedIndex];
                data.ditherPatterns[paletteIndex][static_cast<size_t>((y << 2) | x)] = quantizedColor.ToPixel();
            }
        }
    }

    return data;
}

std::array<Color, kPico8PaletteSize> ConvertCustomPalette(const std::array<Pixel, kPico8PaletteSize>& pixels) {
    std::array<Color, kPico8PaletteSize> colors{};
    for (size_t i = 0; i < colors.size(); i++) {
        colors[i] = MakeColor(pixels[i]);
    }
    return colors;
}

const PaletteData& GetBuiltInPaletteData(Config::PaletteType palette) {
    switch (NormalizeBuiltInPalette(palette)) {
    case Config::PaletteType::PICO8: {
        static const PaletteData data = BuildPaletteData(kPico8Palette, &kPico8Ramps);
        return data;
    }
    case Config::PaletteType::DB16: {
        static const PaletteData data = BuildPaletteData(kDb16Palette);
        return data;
    }
    case Config::PaletteType::SWEETIE16: {
        static const PaletteData data = BuildPaletteData(kSweetie16Palette);
        return data;
    }
    case Config::PaletteType::CUSTOM:
    case Config::PaletteType::NONE:
        break;
    }

    static const PaletteData fallback = BuildPaletteData(kPico8Palette, &kPico8Ramps);
    return fallback;
}

const PaletteData& GetPaletteData(const Config::RetroStyleSettings& retro) {
    if (retro.palette != Config::PaletteType::CUSTOM) {
        return GetBuiltInPaletteData(retro.palette);
    }

    static uint64_t cachedRevision = 0;
    static PaletteData cachedData = BuildPaletteData(ConvertCustomPalette(Config::DefaultCustomPalette()));
    if (cachedRevision != retro.customPaletteRevision) {
        cachedData = BuildPaletteData(ConvertCustomPalette(retro.customPalette));
        cachedRevision = retro.customPaletteRevision;
    }
    return cachedData;
}

const PaletteData& GetPaletteData(Config::PaletteType palette) {
    return GetBuiltInPaletteData(palette);
}

} // namespace

const std::array<Color, kPico8PaletteSize>& GetPico8Palette() {
    return kPico8Palette;
}

const PaletteRamp& GetPico8Ramp(uint8_t paletteIndex) {
    return kPico8Ramps[std::min<size_t>(paletteIndex, kPico8Ramps.size() - 1)];
}

const std::array<Color, kPico8PaletteSize>& GetPaletteColors(const Config::RetroStyleSettings& retro) {
    return GetPaletteData(retro).colors;
}

const Color& GetPaletteColor(Config::PaletteType palette, size_t index) {
    const auto& colors = GetPaletteData(palette).colors;
    return colors[std::min(index, colors.size() - 1)];
}

const Color& GetPaletteColor(const Config::RetroStyleSettings& retro, size_t index) {
    const auto& colors = GetPaletteData(retro).colors;
    return colors[std::min(index, colors.size() - 1)];
}

const std::array<Pixel, 16>& GetOrderedDitherPattern4x4(Config::PaletteType palette, uint8_t paletteIndex) {
    const auto& patterns = GetPaletteData(palette).ditherPatterns;
    return patterns[std::min<size_t>(paletteIndex, patterns.size() - 1)];
}

const std::array<Pixel, 16>& GetOrderedDitherPattern4x4(const Config::RetroStyleSettings& retro, uint8_t paletteIndex) {
    const auto& patterns = GetPaletteData(retro).ditherPatterns;
    return patterns[std::min<size_t>(paletteIndex, patterns.size() - 1)];
}

uint8_t FindNearestPaletteIndex(const Color& color, Config::PaletteType palette) {
    if (palette == Config::PaletteType::NONE) {
        return 0;
    }
    return FindNearestPaletteIndex(color.r, color.g, color.b, palette);
}

uint8_t FindNearestPaletteIndex(uint8_t r, uint8_t g, uint8_t b, Config::PaletteType palette) {
    if (palette == Config::PaletteType::NONE) {
        return 0;
    }
    return GetPaletteData(palette).nearestIndexLut[QuantizedRgb5Index(r, g, b)];
}

uint8_t FindNearestPaletteIndex(const Color& color, const Config::RetroStyleSettings& retro) {
    if (retro.palette == Config::PaletteType::NONE) {
        return 0;
    }
    return FindNearestPaletteIndex(color.r, color.g, color.b, retro);
}

uint8_t FindNearestPaletteIndex(uint8_t r, uint8_t g, uint8_t b, const Config::RetroStyleSettings& retro) {
    if (retro.palette == Config::PaletteType::NONE) {
        return 0;
    }
    return GetPaletteData(retro).nearestIndexLut[QuantizedRgb5Index(r, g, b)];
}

Color FindNearestPaletteColor(const Color& color, Config::PaletteType palette) {
    if (palette == Config::PaletteType::NONE) {
        return color;
    }
    const Color& quantized = GetPaletteColor(palette, FindNearestPaletteIndex(color.r, color.g, color.b, palette));
    return PreserveAlpha(color, quantized);
}

Color FindNearestPaletteColor(const Color& color, const Config::RetroStyleSettings& retro) {
    if (retro.palette == Config::PaletteType::NONE) {
        return color;
    }
    const Color& quantized = GetPaletteColor(retro, FindNearestPaletteIndex(color.r, color.g, color.b, retro));
    return PreserveAlpha(color, quantized);
}

Pixel FindNearestPalettePixel(const Color& color, Config::PaletteType palette) {
    if (palette == Config::PaletteType::NONE) {
        return color.ToPixel();
    }
    const Color& quantized = GetPaletteColor(palette, FindNearestPaletteIndex(color.r, color.g, color.b, palette));
    return Pixel{quantized.r, quantized.g, quantized.b, color.a};
}

Pixel FindNearestPalettePixel(const Color& color, const Config::RetroStyleSettings& retro) {
    if (retro.palette == Config::PaletteType::NONE) {
        return color.ToPixel();
    }
    const Color& quantized = GetPaletteColor(retro, FindNearestPaletteIndex(color.r, color.g, color.b, retro));
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

    if (strength == 1.0f) {
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

Color ApplyOrderedDither4x4(const Color& color,
                            const glm::ivec2& pixelPos,
                            const Config::RetroStyleSettings& retro,
                            float strength) {
    if (retro.palette == Config::PaletteType::NONE) {
        return color;
    }

    if (strength == 1.0f) {
        const uint8_t paletteIndex = FindNearestPaletteIndex(color.r, color.g, color.b, retro);
        const Pixel pixel = GetOrderedDitherPattern4x4(retro, paletteIndex)[static_cast<size_t>(((pixelPos.y & 3) << 2) | (pixelPos.x & 3))];
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
    return FindNearestPaletteColor(adjusted, retro);
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
    const auto& ramps = GetPaletteData(palette).ramps;
    const PaletteRamp& ramp = ramps[std::min<size_t>(basePaletteIndex, ramps.size() - 1)];
    const float quantized = QuantizeUnitToBands(value, bandCount);
    const size_t slot =
        std::min<size_t>(static_cast<size_t>(std::lround(quantized * static_cast<float>(ramp.paletteIndices.size() - 1))),
                         ramp.paletteIndices.size() - 1);
    return GetPaletteColor(palette, ramp.paletteIndices[slot]);
}

Color SampleRamp(const Config::RetroStyleSettings& retro, uint8_t basePaletteIndex, float value, int bandCount) {
    const auto& ramps = GetPaletteData(retro).ramps;
    const PaletteRamp& ramp = ramps[std::min<size_t>(basePaletteIndex, ramps.size() - 1)];
    const float quantized = QuantizeUnitToBands(value, bandCount);
    const size_t slot =
        std::min<size_t>(static_cast<size_t>(std::lround(quantized * static_cast<float>(ramp.paletteIndices.size() - 1))),
                         ramp.paletteIndices.size() - 1);
    return GetPaletteColor(retro, ramp.paletteIndices[slot]);
}

Pixel SampleRampPixel(Config::PaletteType palette,
                      uint8_t basePaletteIndex,
                      float value,
                      int bandCount,
                      uint8_t alpha) {
    const Color rampColor = SampleRamp(palette, basePaletteIndex, value, bandCount);
    return Pixel{rampColor.r, rampColor.g, rampColor.b, alpha};
}

Pixel SampleRampPixel(const Config::RetroStyleSettings& retro,
                      uint8_t basePaletteIndex,
                      float value,
                      int bandCount,
                      uint8_t alpha) {
    const Color rampColor = SampleRamp(retro, basePaletteIndex, value, bandCount);
    return Pixel{rampColor.r, rampColor.g, rampColor.b, alpha};
}

void CopyPaletteToCustom(Config::RetroStyleSettings& retro, Config::PaletteType sourcePalette) {
    const auto& paletteColors = GetPaletteData(sourcePalette).colors;
    for (size_t i = 0; i < paletteColors.size(); i++) {
        retro.customPalette[i] = paletteColors[i].ToPixel();
    }
    retro.customPaletteRevision++;
}

} // namespace RetroPalette
} // namespace RetroRenderer
