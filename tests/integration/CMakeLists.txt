# Integration tests — require engine runtime (Application, Scene, Physics, Renderer, Scripting)
set(INTEGRATION_SOURCES
    scene_tests.cpp
    scene_serialization_tests.cpp
    physics_tests.cpp
    scriptengine_tests.cpp
    renderer_tests.cpp
    audio_tests.cpp
    importer_tests.cpp
    integration_tests.cpp
    asset_manager_tests.cpp
    primitive_tests.cpp
    material_tests.cpp
    hierarchy_tests.cpp
    serialization_tests.cpp
    smoke_tests.cpp
    regression_tests.cpp
    test_environment.cpp
    test_main.cpp
)

add_executable(engine_tests_integration ${INTEGRATION_SOURCES})

target_link_libraries(engine_tests_integration PRIVATE
    engine_core
    engine_app
    engine_scene
    engine_graphics
    engine_physics
    engine_audio
    engine_scripting
    engine_project
    engine_assets
    engine_external_imgui
    engine_external_entt
    engine_reflection
    gtest
    gtest_main
)

# MinGW/gcc: statically link the gcc/stdc++ runtime so the test exe can start
# during test discovery without libstdc++-6.dll / libgcc_s / libwinpthread on PATH.
if(MINGW)
    target_link_options(engine_tests_integration PRIVATE -static-libgcc -static-libstdc++)
endif()

target_include_directories(engine_tests_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
)

include(GoogleTest)
# Integration tests run as a SINGLE ctest entry (one process for the whole
# binary) rather than via gtest_discover_tests. With PRE_TEST discovery every
# test case became its own process, and each process paid a full engine boot
# (Audio + Jolt + optionally CoreCLR) in the global test environment — 80+
# cold starts per ctest run, and a single crash in shared startup code fanned
# out into 80+ identical failures wrapped in unreadable LaunchTest.cmake
# errors. One process keeps gtest's own per-test reporting and readable crash
# output. Unit tests (fast, no engine boot) keep per-test discovery.
add_test(NAME IntegrationSuite COMMAND engine_tests_integration)
set_tests_properties(IntegrationSuite PROPERTIES
    LABELS "Integration"
    ENVIRONMENT "ENGINE_ROOT=${CMAKE_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    TIMEOUT 300
)