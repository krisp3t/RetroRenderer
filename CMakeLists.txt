cmake_minimum_required(VERSION 3.18)
project(retrorenderer VERSION 1.0 LANGUAGES C CXX)

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

    # Debug flags
    add_compile_options(
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Debug>:-O0>
    )
endif()

# -----------------------------------------------------------------------------
# Dependencies (vcpkg)
# -----------------------------------------------------------------------------
find_package(SDL2      CONFIG REQUIRED)
find_package(glm       CONFIG REQUIRED)
find_package(imgui     CONFIG REQUIRED)
find_package(assimp    CONFIG REQUIRED)
find_package(glad      CONFIG REQUIRED)


# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
set(SOURCE_ROOT_DIRS
        src/
        src/Base
        src/Renderer
        src/Renderer/OpenGL
        src/Renderer/Software
        src/Scene
        src/Window
)
set(SOURCES "")
foreach (dir ${SOURCE_ROOT_DIRS})
    file(GLOB_RECURSE CURR_SOURCES CONFIGURE_DEPENDS
            "${dir}/*.cpp"
            "${dir}/*.h"
    )
    list(APPEND SOURCES ${CURR_SOURCES})
endforeach ()
add_subdirectory(lib/KrisLogger)

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
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
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
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL=0)
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# -----------------------------------------------------------------------------
# Libraries linkage
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        imgui::imgui
        glm::glm
        assimp::assimp
        glad::glad
        KrisLogger
)

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
