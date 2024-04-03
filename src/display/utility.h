#include <stdint.h>
#include <glm/vec2.hpp>

namespace MiniRenderer 
{
	uint32_t rgbaToHex(float(&fg_color)[4]);
	glm::vec2 NDC_to_Screen(const glm::vec2& ndc, const int winWidth, const int winHeight);
}