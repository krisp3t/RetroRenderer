# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

set(ANDROID_NDK $ENV{ANDROID_NDK})
file(TO_CMAKE_PATH "${ANDROID_NDK}" ANDROID_NDK) # Convert to forward slashes
add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue)

project(retrorenderer C CXX)

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(retrorenderer SHARED
        src/main/cpp/template.cpp
        src/main/cpp/game.cpp
        src/main/cpp/surface.cpp
)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Configure libraries CMake uses to link your target library.
target_link_libraries(retrorenderer
        # The game activity
        game-activity::game-activity

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        native_app_glue
        EGL
        GLESv3
        jnigraphics
        android
        log)