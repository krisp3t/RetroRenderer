#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <imgui.h>

namespace RetroRenderer {
struct Pixel {
    // We support different endianness systems, so we can't rely on uint32_t.
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct Color {
    uint8_t r, g, b, a;

    // Tag structs for constructor differentiation
    struct Uint8Tag {
    };

    struct FloatTag {
    };

    Color() : r(0), g(0), b(0), a(255) {
    }

    Color(const Color& c) : r(c.r), g(c.g), b(c.b), a(c.a) {
    }

    explicit Color(Uint8Tag, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {
    }

    explicit Color(FloatTag, float red, float green, float blue, float alpha = 1.0f)
        : r(static_cast<uint8_t>(glm::clamp(red, 0.0f, 1.0f) * 255.0f)),
          g(static_cast<uint8_t>(glm::clamp(green, 0.0f, 1.0f) * 255.0f)),
          b(static_cast<uint8_t>(glm::clamp(blue, 0.0f, 1.0f) * 255.0f)),
          a(static_cast<uint8_t>(glm::clamp(alpha, 0.0f, 1.0f) * 255.0f)) {
    }

    explicit Color(uint8_t gray, uint8_t alpha = 255) : r(gray), g(gray), b(gray), a(alpha) {
    }

    explicit Color(const ImVec4& c)
        : r(static_cast<uint8_t>(glm::clamp(c.x, 0.0f, 1.0f) * 255.0f)),
          g(static_cast<uint8_t>(glm::clamp(c.y, 0.0f, 1.0f) * 255.0f)),
          b(static_cast<uint8_t>(glm::clamp(c.z, 0.0f, 1.0f) * 255.0f)),
          a(static_cast<uint8_t>(glm::clamp(c.w, 0.0f, 1.0f) * 255.0f)) {
    }

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool operator!=(const Color& other) const {
        return !(*this == other);
    }

    [[nodiscard]] Pixel ToPixel() const {
        return Pixel{r, g, b, a};
    }

    [[nodiscard]] ImVec4 ToImVec4() const {
        return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }

    // Pre-defined colors
    static Color DefaultBackground() {
        return Color(Color::Uint8Tag{}, 47, 42, 85); // purple-ish
    }

    static Color Black() {
        return Color(Color::Uint8Tag{}, 0, 0, 0);
    }

    static Color White() {
        return Color(Color::Uint8Tag{}, 255, 255, 255);
    }

    static Color Red() {
        return Color(Color::Uint8Tag{}, 255, 0, 0);
    }

    static Color Green() {
        return Color(Color::Uint8Tag{}, 0, 255, 0);
    }

    static Color Blue() {
        return Color(Color::Uint8Tag{}, 0, 0, 255);
    }

    static Color Yellow() {
        return Color(Color::Uint8Tag{}, 255, 255, 0);
    }

    static Color Cyan() {
        return Color(Color::Uint8Tag{}, 0, 255, 255);
    }

    static Color Magenta() {
        return Color(Color::Uint8Tag{}, 255, 0, 255);
    }

    static Color Gray() {
        return Color(Color::Uint8Tag{}, 128, 128, 128);
    }

    static Color DarkGray() {
        return Color(Color::Uint8Tag{}, 64, 64, 64);
    }

    static Color LightGray() {
        return Color(Color::Uint8Tag{}, 192, 192, 192);
    }

    static Color Orange() {
        return Color(Color::Uint8Tag{}, 255, 165, 0);
    }

    static Color Purple() {
        return Color(Color::Uint8Tag{}, 128, 0, 128);
    }

    static Color Brown() {
        return Color(Color::Uint8Tag{}, 139, 69, 19);
    }

    static Color Pink() {
        return Color(Color::Uint8Tag{}, 255, 192, 203);
    }

    static Color Teal() {
        return Color(Color::Uint8Tag{}, 0, 128, 128);
    }

    // TODO: add blending (alpha compositing), premultiply alpha
};
} // namespace RetroRenderer
