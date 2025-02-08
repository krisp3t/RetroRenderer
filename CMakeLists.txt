cmake_minimum_required (VERSION 3.8)

if (ANDROID)
	if(NOT DEFINED ENV{ANDROID_NDK_HOME})
		message(FATAL_ERROR "ANDROID_NDK_HOME is not defined. Please set the environment variable or configure it in CMakePresets.")
	elseif("$ENV{ANDROID_NDK_HOME}" STREQUAL "")
		message(FATAL_ERROR "ANDROID_NDK_HOME is set to an empty value. "
			"Please provide a valid Android NDK path.")
	else()
		message(STATUS "Using ANDROID_NDK_HOME in: $ENV{ANDROID_NDK_HOME}")
	endif()

	#unset(CMAKE_C_COMPILER)
	#unset(CMAKE_CXX_COMPILER)
	set(VCPKG_TARGET_TRIPLET "${CONFIG_ANDROID_TRIPLET}" CACHE STRING "vcpkg target triplet")
	set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}")
	set(CMAKE_TOOLCHAIN_FILE "$ENV{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake")
endif()

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("RetroRenderer")

find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(glad REQUIRED)

file(GLOB_RECURSE SOURCES src/*.h src/*.cpp)
add_subdirectory(lib/KrisLogger)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} 
				PRIVATE 
				${SOURCES} 
				${CMAKE_CURRENT_SOURCE_DIR}/lib/ImGuiFileDialog/ImGuiFileDialog.cpp
)
target_include_directories(${PROJECT_NAME} 
							PRIVATE 
							${CMAKE_CURRENT_SOURCE_DIR}/src
							${CMAKE_CURRENT_SOURCE_DIR}/lib/ImGuiFileDialog
)

# target_compile_options(${PROJECT_NAME} PRIVATE
#    $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX /Ot /fp:fast>
#    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -pedantic -Werror>
#)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME}
    PRIVATE
	SDL2::SDL2main
	SDL2::SDL2
    imgui::imgui
    glm::glm
    OpenGL::GL
    assimp::assimp
    glad::glad
	KrisLogger
)