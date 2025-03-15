#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__i386__) || defined(__x86_64__)
#define DEBUG_BREAK() __asm__ __volatile__("int $3")
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
