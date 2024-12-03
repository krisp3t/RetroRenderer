#pragma once

namespace RetroRenderer
{
	struct Color
	{
		uint8_t r, g, b, a;

		// Constructors
		Color() : r(0), g(0), b(0), a(255) {} // Default: opaque black
		Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
			: r(red), g(green), b(blue), a(alpha) {}

		// Pre-defined colors
		static const Color Black() { return Color(0, 0, 0); }
		static const Color White() { return Color(255, 255, 255); }
		static const Color Red() { return Color(255, 0, 0); }
		static const Color Green() { return Color(0, 255, 0); }
		static const Color Blue() { return Color(0, 0, 255); }
		static const Color Yellow() { return Color(255, 255, 0); }
		static const Color Cyan() { return Color(0, 255, 255); }
		static const Color Magenta() { return Color(255, 0, 255); }
		static const Color Gray() { return Color(128, 128, 128); }
		static const Color DarkGray() { return Color(64, 64, 64); }
		static const Color LightGray() { return Color(192, 192, 192); }
		static const Color Orange() { return Color(255, 165, 0); }
		static const Color Purple() { return Color(128, 0, 128); }
		static const Color Brown() { return Color(139, 69, 19); }
		static const Color Pink() { return Color(255, 192, 203); }
		static const Color Teal() { return Color(0, 128, 128); }

		// From uint32_t in ARGB8888 format
		static Color FromARGB(uint32_t argb)
		{
			return Color(
				(argb >> 16) & 0xFF, // Red
				(argb >> 8) & 0xFF,  // Green
				argb & 0xFF,         // Blue
				(argb >> 24) & 0xFF  // Alpha
			);
		}

		// From uint32_t in RGBA8888 format
		static Color FromRGBA(uint32_t rgba)
		{
			return Color(
				(rgba >> 24) & 0xFF, // Red
				(rgba >> 16) & 0xFF, // Green
				(rgba >> 8) & 0xFF,  // Blue
				rgba & 0xFF          // Alpha
			);
		}

		// To uint32_t in ARGB8888 format
		uint32_t ToARGB() const
		{
			return (a << 24) | (r << 16) | (g << 8) | b;
		}

		// To uint32_t in RGBA8888 format
		uint32_t ToRGBA() const
		{
			return (r << 24) | (g << 16) | (b << 8) | a;
		}

		// Helper to blend two colors (alpha compositing)
		static Color Blend(const Color& src, const Color& dest)
		{
			float srcAlpha = src.a / 255.0f;
			float invAlpha = 1.0f - srcAlpha;

			return Color(
				std::clamp(static_cast<int>(src.r * srcAlpha + dest.r * invAlpha), 0, 255),
				std::clamp(static_cast<int>(src.g * srcAlpha + dest.g * invAlpha), 0, 255),
				std::clamp(static_cast<int>(src.b * srcAlpha + dest.b * invAlpha), 0, 255),
				std::clamp(static_cast<int>(src.a + dest.a * invAlpha), 0, 255)
			);
		}

		// Premultiply alpha for optimization
		Color PremultiplyAlpha() const
		{
			return Color(
				static_cast<uint8_t>(r * (a / 255.0f)),
				static_cast<uint8_t>(g * (a / 255.0f)),
				static_cast<uint8_t>(b * (a / 255.0f)),
				a
			);
		}

		// Comparison operators
		bool operator==(const Color& other) const
		{
			return r == other.r && g == other.g && b == other.b && a == other.a;
		}

		bool operator!=(const Color& other) const
		{
			return !(*this == other);
		}
	};
}