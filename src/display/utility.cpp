#include "utility.h"

namespace MiniRenderer {
	uint32_t rgbaToHex(float fg_color[4]) {
		short r = fg_color[0] * 255;
		short g = fg_color[1] * 255;
		short b = fg_color[2] * 255;
		short a = fg_color[3] * 255;

		return (a << 24) | (r << 16) | (g << 8) | b;
	}
}
