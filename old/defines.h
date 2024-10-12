#pragma once

#ifdef __AVX__
#include <immintrin.h>
#define AVX_SUPPORTED true
#else
#define AVX_SUPPORTED false
#endif

namespace KrisRenderer
{
	constexpr int GL_MAJOR = 3; // do not name GL_MAJOR_VERSION! it is SDL's macro
	constexpr int GL_MINOR = 3;
	constexpr auto GLSL_VERSION = "#version 330";
	constexpr int DEFAULT_WIN_WIDTH = 1280;
	constexpr int DEFAULT_WIN_HEIGHT = 720;
}


