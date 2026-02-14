cmake_minimum_required(VERSION 3.18)
project(retrorenderer VERSION 1.0 LANGUAGES C CXX)
option(RETRO_ENABLE_ASSIMP "Enable Assimp scene importer backend" ON)
option(RETRO_BUILD_TESTS "Build Catch2 unit tests" OFF)
option(RETRO_REQUIRE_CLANG "Fail configure if Clang is not the active compiler" OFF)
set(RETRO_SANITIZERS "" CACHE STRING "Semicolon-separated sanitizers for Clang/GCC (address;undefined;thread;leak)")

if(RETRO_REQUIRE_CLANG)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR
            "RETRO_REQUIRE_CLANG=ON but active compilers are C='${CMAKE_C_COMPILER_ID}', "
            "CXX='${CMAKE_CXX_COMPILER_ID}'. Configure with clang/clang++.")
    endif()
endif()

# -----------------------------------------------------------------------------
# Android toolchain (Gradle will inject ANDROID_ABI, ANDROID_PLATFORM)
# -----------------------------------------------------------------------------
if(ANDROID)
    message(STATUS "Configuring for Android")
    set(CMAKE_SYSTEM_NAME Android)
    set(CMAKE_ANDROID_NDK            $ENV{ANDROID_NDK_HOME})
    set(CMAKE_ANDROID_ARCH_ABI       arm64-v8a)
    set(CMAKE_ANDROID_API            26)
    set(CMAKE_ANDROID_STL_TYPE       c++_shared)

    set(VCPKG_TARGET_TRIPLET arm64-android)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/extern/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "$ENV{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake")

    set(NATIVE_FILES
            src/native/android/AndroidBridge.h
            src/native/android/AndroidBridge.cpp
    )
    set(RENDERER_BACKENDS
            src/Renderer/GLES/GLESRenderer.h
            src/Renderer/GLES/GLESRenderer.cpp
    )

    # Debug flags
    add_compile_options(
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Debug>:-O0>
    )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(RENDERER_BACKENDS
            src/Renderer/GLES/GLESRenderer.h
            src/Renderer/GLES/GLESRenderer.cpp
    )
    set(NATIVE_FILES
            src/native/emscripten/EmscriptenBridge.h
            src/native/emscripten/imgui_impl_sdl2.h
            src/native/emscripten/imgui_impl_sdl2.cpp
    )
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION assets/)
    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/index.html
            ${CMAKE_CURRENT_BINARY_DIR}/index.html
            COPYONLY
    )
else()
    set(RENDERER_BACKENDS
            src/Renderer/OpenGL/GLRenderer.h
            src/Renderer/OpenGL/GLRenderer.cpp
    )
endif()
# -----------------------------------------------------------------------------
# Dependencies (vcpkg)
# -----------------------------------------------------------------------------
if(NOT (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten"))
find_package(SDL2      CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
endif ()
find_package(glm       CONFIG REQUIRED)
find_package(imgui     CONFIG REQUIRED)
if(RETRO_ENABLE_ASSIMP)
    find_package(assimp CONFIG REQUIRED)
    set(RETRO_WITH_ASSIMP_VALUE 1)
else()
    set(RETRO_WITH_ASSIMP_VALUE 0)
endif()
find_package(glad      CONFIG REQUIRED)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
set(SOURCE_ROOT_DIRS
        src/
        src/Base
        src/Renderer
        src/Renderer/Software
        src/Scene
        src/Window
        src/include
)
set(SOURCES "")
foreach (dir ${SOURCE_ROOT_DIRS})
    file(GLOB CURR_SOURCES CONFIGURE_DEPENDS
            "${dir}/*.cpp"
            "${dir}/*.h"
    )
    list(APPEND SOURCES ${CURR_SOURCES})
endforeach ()
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/KrisLogger)

add_library(retro_sanitizers INTERFACE)
set(RETRO_WINDOWS_ASAN_RUNTIME_DLL "")
set(RETRO_WINDOWS_UBSAN_RUNTIME_DLL "")
set(RETRO_WINDOWS_SANITIZER_RUNTIME_DLLS "")

function(retro_find_windows_clang_runtime_dll runtime_name out_var)
    set(retro_runtime_dll_path "")

    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${runtime_name}
        OUTPUT_VARIABLE RETRO_CLANG_RUNTIME_PRINT_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(RETRO_CLANG_RUNTIME_PRINT_PATH AND EXISTS "${RETRO_CLANG_RUNTIME_PRINT_PATH}")
        set(retro_runtime_dll_path "${RETRO_CLANG_RUNTIME_PRINT_PATH}")
    endif()

    if(NOT retro_runtime_dll_path)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} --print-runtime-dir
            OUTPUT_VARIABLE RETRO_CLANG_RUNTIME_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(RETRO_CLANG_RUNTIME_DIR)
            find_file(retro_runtime_dll_path
                NAMES ${runtime_name}
                PATHS "${RETRO_CLANG_RUNTIME_DIR}"
                NO_DEFAULT_PATH
            )
        endif()
    endif()

    if(NOT retro_runtime_dll_path)
        get_filename_component(RETRO_CLANG_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        find_file(retro_runtime_dll_path
            NAMES ${runtime_name}
            PATHS "${RETRO_CLANG_BIN_DIR}"
            NO_DEFAULT_PATH
        )
    endif()

    if(NOT retro_runtime_dll_path)
        get_filename_component(RETRO_CLANG_ROOT_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(RETRO_CLANG_ROOT_DIR "${RETRO_CLANG_ROOT_DIR}" DIRECTORY)
        file(GLOB RETRO_CLANG_RUNTIME_DLL_CANDIDATES
            "${RETRO_CLANG_ROOT_DIR}/lib/clang/*/lib/windows/${runtime_name}"
        )
        list(LENGTH RETRO_CLANG_RUNTIME_DLL_CANDIDATES RETRO_CLANG_RUNTIME_DLL_COUNT)
        if(RETRO_CLANG_RUNTIME_DLL_COUNT GREATER 0)
            list(SORT RETRO_CLANG_RUNTIME_DLL_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
            list(GET RETRO_CLANG_RUNTIME_DLL_CANDIDATES 0 retro_runtime_dll_path)
        endif()
    endif()

    set(${out_var} "${retro_runtime_dll_path}" PARENT_SCOPE)
endfunction()

if(RETRO_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "RETRO_SANITIZERS requires a Clang or GCC toolchain.")
    endif()

    set(RETRO_SANITIZER_FLAGS "")
    set(RETRO_HAS_THREAD_SAN OFF)
    set(RETRO_HAS_ADDRESS_SAN OFF)
    set(RETRO_HAS_UNDEFINED_SAN OFF)
    set(RETRO_SANITIZER_COUNT 0)
    foreach(RETRO_SANITIZER IN LISTS RETRO_SANITIZERS)
        string(TOLOWER "${RETRO_SANITIZER}" RETRO_SANITIZER_LOWER)
        if(NOT RETRO_SANITIZER_LOWER MATCHES "^(address|undefined|thread|leak)$")
            message(FATAL_ERROR "Unsupported sanitizer '${RETRO_SANITIZER}'. Allowed values: address;undefined;thread;leak")
        endif()
        if(RETRO_SANITIZER_LOWER STREQUAL "thread")
            set(RETRO_HAS_THREAD_SAN ON)
        endif()
        if(RETRO_SANITIZER_LOWER STREQUAL "address")
            set(RETRO_HAS_ADDRESS_SAN ON)
        endif()
        if(RETRO_SANITIZER_LOWER STREQUAL "undefined")
            set(RETRO_HAS_UNDEFINED_SAN ON)
        endif()
        math(EXPR RETRO_SANITIZER_COUNT "${RETRO_SANITIZER_COUNT} + 1")
        list(APPEND RETRO_SANITIZER_FLAGS "-fsanitize=${RETRO_SANITIZER_LOWER}")
    endforeach()

    if(RETRO_HAS_THREAD_SAN AND RETRO_SANITIZER_COUNT GREATER 1)
        message(FATAL_ERROR "ThreadSanitizer cannot be combined with other sanitizers in RETRO_SANITIZERS.")
    endif()

    target_compile_options(retro_sanitizers INTERFACE
        -fno-omit-frame-pointer
        ${RETRO_SANITIZER_FLAGS}
    )
    target_link_options(retro_sanitizers INTERFACE
        ${RETRO_SANITIZER_FLAGS}
    )

    if(WIN32 AND RETRO_HAS_UNDEFINED_SAN)
        # Clang's Windows UBSan C++ runtime (ubsan_standalone_cxx) is built
        # with static CRT and conflicts with /MD third-party libraries.
        # Disable vptr instrumentation to avoid linking that runtime while
        # keeping the rest of UBSan checks enabled.
        target_compile_options(retro_sanitizers INTERFACE -fno-sanitize=vptr)
        target_link_options(retro_sanitizers INTERFACE -fno-sanitize=vptr)
        message(STATUS "Windows UBSan: disabled vptr checks to avoid CRT runtime mismatch.")
    endif()

    if(WIN32 AND RETRO_HAS_ADDRESS_SAN)
        # Prebuilt third-party Windows libs are commonly non-ASan.
        # Disable STL container/string annotations to avoid /failifmismatch
        # while keeping core ASan instrumentation enabled.
        target_compile_definitions(retro_sanitizers INTERFACE
            _DISABLE_VECTOR_ANNOTATION
            _DISABLE_STRING_ANNOTATION
        )

        retro_find_windows_clang_runtime_dll("clang_rt.asan_dynamic-x86_64.dll" RETRO_WINDOWS_ASAN_RUNTIME_DLL)
        if(NOT RETRO_WINDOWS_ASAN_RUNTIME_DLL)
            message(WARNING "Windows ASan enabled, but clang_rt.asan_dynamic-x86_64.dll was not found. Tests may fail with 0xc0000135.")
        else()
            message(STATUS "Windows ASan runtime: ${RETRO_WINDOWS_ASAN_RUNTIME_DLL}")
            list(APPEND RETRO_WINDOWS_SANITIZER_RUNTIME_DLLS "${RETRO_WINDOWS_ASAN_RUNTIME_DLL}")
        endif()
    endif()

    if(WIN32 AND RETRO_HAS_UNDEFINED_SAN)
        retro_find_windows_clang_runtime_dll("clang_rt.ubsan_standalone-x86_64.dll" RETRO_WINDOWS_UBSAN_RUNTIME_DLL)
        if(NOT RETRO_WINDOWS_UBSAN_RUNTIME_DLL)
            message(STATUS "Windows UBSan dynamic runtime not found; using static UBSan runtime linkage.")
        else()
            message(STATUS "Windows UBSan runtime: ${RETRO_WINDOWS_UBSAN_RUNTIME_DLL}")
            list(APPEND RETRO_WINDOWS_SANITIZER_RUNTIME_DLLS "${RETRO_WINDOWS_UBSAN_RUNTIME_DLL}")
        endif()
    endif()

    if(RETRO_WINDOWS_SANITIZER_RUNTIME_DLLS)
        list(REMOVE_DUPLICATES RETRO_WINDOWS_SANITIZER_RUNTIME_DLLS)
    endif()

    message(STATUS "Enabled sanitizers: ${RETRO_SANITIZERS}")
endif()
target_link_libraries(KrisLogger PUBLIC retro_sanitizers)

function(retro_copy_windows_sanitizer_runtime target_name)
    if(WIN32 AND RETRO_WINDOWS_SANITIZER_RUNTIME_DLLS)
        foreach(retro_runtime_dll IN LISTS RETRO_WINDOWS_SANITIZER_RUNTIME_DLLS)
            if(EXISTS "${retro_runtime_dll}")
                add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${retro_runtime_dll}"
                        "$<TARGET_FILE_DIR:${target_name}>"
                    VERBATIM
                )
            endif()
        endforeach()
    endif()
endfunction()

function(retro_copy_windows_asan_runtime target_name)
    retro_copy_windows_sanitizer_runtime(${target_name})
endfunction()

# -----------------------------------------------------------------------------
# Native glue for Android
# -----------------------------------------------------------------------------
if (ANDROID)
    add_library(native_app_glue STATIC
            ${ANDROID_NDK_HOME}/sources/android/native_app_glue/android_native_app_glue.c)
    target_include_directories(native_app_glue PUBLIC
            ${ANDROID_NDK_HOME}/sources/android/native_app_glue)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
endif()

# -----------------------------------------------------------------------------
# Main target
# -----------------------------------------------------------------------------
if(ANDROID)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${RENDERER_BACKENDS} ${NATIVE_FILES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${RENDERER_BACKENDS} ${NATIVE_FILES})
    retro_copy_windows_sanitizer_runtime(${PROJECT_NAME})
endif()

# -----------------------------------------------------------------------------
# Extra sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/ImGuiFileDialog/ImGuiFileDialog.cpp
)
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/ImGuiFileDialog.h
)

# -----------------------------------------------------------------------------
# C/C++ standards & warnings
# -----------------------------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
        C_STANDARD 11
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL=0 RETRO_WITH_ASSIMP=${RETRO_WITH_ASSIMP_VALUE})
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    target_link_options(${PROJECT_NAME}  PRIVATE
            "-sEXPORTED_FUNCTIONS=['_main','_malloc','_free']"
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','HEAPU8']"
    )
endif()

# -----------------------------------------------------------------------------
# Libraries linkage
# -----------------------------------------------------------------------------
if(NOT (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten"))
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
)
endif()
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        imgui::imgui
        glm::glm
        glad::glad
        KrisLogger
        retro_sanitizers
)
if(RETRO_ENABLE_ASSIMP)
    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            assimp::assimp
    )
endif()
# -----------------------------------------------------------------------------
# Android-specific system libs
# -----------------------------------------------------------------------------
if(ANDROID)
    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            android
            log
            EGL
            GLESv3
            jnigraphics
            native_app_glue
    )
endif()
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            psapi
    )
endif()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
if(RETRO_BUILD_TESTS)
    if(ANDROID OR CMAKE_SYSTEM_NAME MATCHES "Emscripten")
        message(WARNING "RETRO_BUILD_TESTS is only supported on desktop platforms. Skipping tests.")
    else()
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()
