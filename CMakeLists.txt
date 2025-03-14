cmake_minimum_required(VERSION 3.12)
project(retrorenderer VERSION 1.0 LANGUAGES C CXX)

find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp REQUIRED)
find_package(glad REQUIRED)

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

if (ANDROID)
    add_library(native_app_glue STATIC
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
    target_include_directories(native_app_glue PUBLIC
            ${ANDROID_NDK}/sources/android/native_app_glue)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

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
#target_compile_options(${PROJECT_NAME} PRIVATE
#    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
#)

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

if (ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            android
            log
            EGL
            GLESv3
            jnigraphics
            native_app_glue
    )
endif()
