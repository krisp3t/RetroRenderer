#include <stdint.h>
#include <glm/vec2.hpp>

namespace KrisRenderer 
{
	uint32_t rgbaToHexArgb(float(&fg_color)[4]);
	uint32_t rgbaToHexRgba(float(&fg_color)[4]);
	glm::vec2 NDC_to_Screen(const glm::vec2& ndc, const int winWidth, const int winHeight);
}