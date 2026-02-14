find_package(Catch2 CONFIG REQUIRED)

add_executable(retrorenderer_tests
    ${CMAKE_CURRENT_LIST_DIR}/ConcurrencyTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/GoldenRenderingTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/IntegrationTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/LightweightObjSceneImporterTests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RasterizerTests.cpp
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
target_compile_definitions(retrorenderer_tests
    PRIVATE
    RETRO_GOLDEN_HASH_FILE=\"${CMAKE_CURRENT_LIST_DIR}/golden/software_pipeline_hashes.txt\"
)

target_link_libraries(retrorenderer_tests
    PRIVATE
    Catch2::Catch2WithMain
    glm::glm
    KrisLogger
)

include(Catch OPTIONAL RESULT_VARIABLE catchModulePath)
set(retro_test_properties)
if(WIN32)
    # Visual Studio Test Explorer does not reliably use CMake test preset env vars.
    # Set the env at test registration time so golden tests run on Windows by default.
    set(retro_test_properties
        PROPERTIES
        ENVIRONMENT "RETRO_RUN_GOLDEN_WINDOWS=1"
    )
endif()

if(catchModulePath)
    catch_discover_tests(retrorenderer_tests
        DISCOVERY_MODE PRE_TEST
        ${retro_test_properties}
    )
else()
    add_test(NAME retrorenderer_tests COMMAND $<TARGET_FILE:retrorenderer_tests>)
    if(retro_test_properties)
        set_tests_properties(retrorenderer_tests ${retro_test_properties})
    endif()
endif()
