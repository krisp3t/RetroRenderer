set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

# CMake 4 removed compatibility with policy versions older than 3.5.
# Some dependencies pinned by the vcpkg baseline still declare an older
# minimum, so provide the compatibility floor to their configure steps.
set(VCPKG_CMAKE_CONFIGURE_OPTIONS
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
