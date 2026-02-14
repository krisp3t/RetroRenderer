#include <catch2/catch_test_macros.hpp>

#include <iostream>

namespace RetroRenderer {
namespace {
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define RETRO_COMPILED_WITH_ASAN 1
#endif
#endif

#if !defined(RETRO_COMPILED_WITH_ASAN) && defined(__SANITIZE_ADDRESS__)
#define RETRO_COMPILED_WITH_ASAN 1
#endif

#if !defined(RETRO_COMPILED_WITH_ASAN)
#define RETRO_COMPILED_WITH_ASAN 0
#endif
} // namespace

TEST_CASE("Sanitizer smoke: ASan instrumentation status", "[sanitizer][asan]") {
#if defined(RETRO_EXPECT_ASAN)
    std::cout << "[sanitizer-smoke] RETRO_EXPECT_ASAN=1, compiled_with_asan=" << RETRO_COMPILED_WITH_ASAN << '\n';
    REQUIRE(RETRO_COMPILED_WITH_ASAN == 1);
#else
    std::cout << "[sanitizer-smoke] RETRO_EXPECT_ASAN=0 (ASan not requested for this build)\n";
    SUCCEED();
#endif
}

} // namespace RetroRenderer
