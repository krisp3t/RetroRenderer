cmake_minimum_required(VERSION 3.12)
set(BUILD_SHARED_LIBS ON)
set(VCPKG_LIBRARY_LINKAGE dynamic)
project(retrorenderer VERSION 1.0 LANGUAGES C CXX)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif ()

# list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/arm64_android/share/sdl2/SDL2Config.cmake)
set(BUILD_SHARED_LIBS ON)
set(VCPKG_LIBRARY_LINKAGE dynamic)
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp REQUIRED)
find_package(glad REQUIRED)

# Only build the shared library for SDL2

set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
set(SDL_CMAKE_DEBUG_POSTFIX "" CACHE STRING "" FORCE)

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
add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/ImGuiFileDialog/ImGuiFileDialog.cpp
)
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/ImGuiFileDialog.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL=0)
# target_compile_options(${PROJECT_NAME} PRIVATE
#    $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /Ot /fp:fast>
#    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -pedantic -Werror>
#)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        SDL2::SDL2
        imgui::imgui
        glm::glm
        assimp::assimp
        glad::glad
        KrisLogger

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        native_app_glue
        EGL
        GLESv3
        jnigraphics
        android
        log
)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
