find_package(Catch2 CONFIG REQUIRED)

add_executable(retrorenderer_tests
    LightweightObjSceneImporterTests.cpp
    RasterizerTests.cpp
    ${CMAKE_SOURCE_DIR}/src/Renderer/Software/Rasterizer.cpp
    ${CMAKE_SOURCE_DIR}/src/Scene/LightweightObjSceneImporter.cpp
)

target_include_directories(retrorenderer_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(retrorenderer_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(retrorenderer_tests PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
)

target_link_libraries(retrorenderer_tests
    PRIVATE
    Catch2::Catch2WithMain
    glm::glm
    KrisLogger
)

include(Catch OPTIONAL RESULT_VARIABLE catchModulePath)
if(catchModulePath)
    catch_discover_tests(retrorenderer_tests
        DISCOVERY_MODE PRE_TEST
    )
else()
    add_test(NAME retrorenderer_tests COMMAND $<TARGET_FILE:retrorenderer_tests>)
endif()
