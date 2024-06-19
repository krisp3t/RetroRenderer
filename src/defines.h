#pragma once

#ifdef __AVX__
#include <immintrin.h>
#define AVX_SUPPORTED true
#else
#define AVX_SUPPORTED false
#endif