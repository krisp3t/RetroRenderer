find_package(Catch2 CONFIG REQUIRED)

set(retro_use_catch_amalgamated OFF)
if(WIN32 AND RETRO_SANITIZERS MATCHES "(^|;)(address|undefined)($|;)")
    set(retro_use_catch_amalgamated ON)
endif()

add_executable(retrorenderer_tests
    ${CMAKE_CURRENT_LIST_DIR}/ConcurrencyTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/GoldenRenderingTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/IntegrationTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/LightweightObjSceneImporterTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RasterizerTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SanitizerSmokeTests.cpp
    ${CMAKE_SOURCE_DIR}/src/Renderer/Software/Rasterizer.cpp
    ${CMAKE_SOURCE_DIR}/src/Scene/LightweightObjSceneImporter.cpp
)

if(retro_use_catch_amalgamated)
    file(GLOB retro_catch_amalgamated_candidates
        "${CMAKE_SOURCE_DIR}/extern/vcpkg/buildtrees/catch2/src/*/extras/catch_amalgamated.cpp"
    )
    list(LENGTH retro_catch_amalgamated_candidates retro_catch_amalgamated_count)
    if(retro_catch_amalgamated_count EQUAL 0)
        message(FATAL_ERROR "Windows ASan/UBSan requires Catch2 amalgamated source. Could not find catch_amalgamated.cpp in extern/vcpkg/buildtrees/catch2/src/*/extras.")
    endif()
    list(SORT retro_catch_amalgamated_candidates COMPARE NATURAL ORDER DESCENDING)
    list(GET retro_catch_amalgamated_candidates 0 retro_catch_amalgamated_cpp)
    get_filename_component(retro_catch_amalgamated_dir "${retro_catch_amalgamated_cpp}" DIRECTORY)

    target_sources(retrorenderer_tests
        PRIVATE
        "${retro_catch_amalgamated_cpp}"
    )
    target_include_directories(retrorenderer_tests
        PRIVATE
        "${retro_catch_amalgamated_dir}"
    )
endif()

if(COMMAND retro_copy_windows_sanitizer_runtime)
    retro_copy_windows_sanitizer_runtime(retrorenderer_tests)
elseif(COMMAND retro_copy_windows_asan_runtime)
    retro_copy_windows_asan_runtime(retrorenderer_tests)
endif()

target_include_directories(retrorenderer_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(retrorenderer_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(WIN32 AND RETRO_SANITIZERS MATCHES "(^|;)undefined($|;)")
    # Windows UBSan runtime is static-CRT; keep test targets on /MT to avoid
    # /failifmismatch with msvcprt (/MD).
    set(retro_windows_ubsan_msvc_runtime "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET retrorenderer_tests PROPERTY MSVC_RUNTIME_LIBRARY "${retro_windows_ubsan_msvc_runtime}")
    if(TARGET KrisLogger)
        set_property(TARGET KrisLogger PROPERTY MSVC_RUNTIME_LIBRARY "${retro_windows_ubsan_msvc_runtime}")
    endif()
endif()

target_compile_options(retrorenderer_tests PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
)
target_compile_definitions(retrorenderer_tests
    PRIVATE
    RETRO_GOLDEN_HASH_FILE=\"${CMAKE_CURRENT_LIST_DIR}/golden/software_pipeline_hashes.txt\"
)
if(RETRO_SANITIZERS MATCHES "(^|;)address($|;)")
    target_compile_definitions(retrorenderer_tests PRIVATE RETRO_EXPECT_ASAN=1)
endif()
if(RETRO_SANITIZERS MATCHES "(^|;)undefined($|;)")
    target_compile_definitions(retrorenderer_tests PRIVATE RETRO_EXPECT_UBSAN=1)
endif()

target_link_libraries(retrorenderer_tests
    PRIVATE
    $<IF:$<TARGET_EXISTS:glm::glm-header-only>,glm::glm-header-only,glm::glm>
    KrisLogger
    retro_sanitizers
)
if(NOT retro_use_catch_amalgamated)
    target_link_libraries(retrorenderer_tests PRIVATE Catch2::Catch2WithMain)
endif()

include(Catch OPTIONAL RESULT_VARIABLE catchModulePath)
set(retro_test_properties)
set(retro_use_catch_discovery ON)
set(retro_enable_windows_goldens OFF)
set(retro_test_environment_entries)
if(WIN32)
    set(retro_enable_windows_goldens ON)
    # Visual Studio Test Explorer does not reliably use CMake test preset env vars.
    # Set the env at test registration time so golden tests run on Windows by default.
    if(RETRO_SANITIZERS MATCHES "(^|;)address($|;)")
        # ASan lane focuses on memory/thread correctness, not golden hash determinism.
        set(retro_enable_windows_goldens OFF)
        list(APPEND retro_test_environment_entries
            "ASAN_OPTIONS=halt_on_error=1:symbolize=1:verbosity=1:print_stats=1"
            "ASAN_SYMBOLIZER_PATH=C:/Program Files/LLVM/bin/llvm-symbolizer.exe"
        )
    endif()
    if(RETRO_SANITIZERS MATCHES "(^|;)undefined($|;)")
        list(APPEND retro_test_environment_entries
            "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
            "UBSAN_SYMBOLIZER_PATH=C:/Program Files/LLVM/bin/llvm-symbolizer.exe"
        )
    endif()

    if(retro_enable_windows_goldens)
        list(APPEND retro_test_environment_entries "RETRO_RUN_GOLDEN_WINDOWS=1")
    endif()
endif()
if(retro_test_environment_entries)
    list(JOIN retro_test_environment_entries ";" retro_test_environment)
    set(retro_test_properties
        PROPERTIES
        ENVIRONMENT "${retro_test_environment}"
    )
endif()

if(catchModulePath AND retro_use_catch_discovery)
    catch_discover_tests(retrorenderer_tests
        # POST_BUILD makes tests visible to Visual Studio Test Explorer right after build.
        DISCOVERY_MODE POST_BUILD
        ${retro_test_properties}
    )
else()
    add_test(NAME retrorenderer_tests COMMAND $<TARGET_FILE:retrorenderer_tests>)
    if(retro_test_properties)
        set_tests_properties(retrorenderer_tests ${retro_test_properties})
    endif()
endif()
