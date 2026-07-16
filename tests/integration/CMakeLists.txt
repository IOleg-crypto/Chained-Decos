# Integration tests — require engine runtime (Application, Scene, Physics, Renderer, Scripting)
set(INTEGRATION_SOURCES
    scene_tests.cpp
    scene_serialization_tests.cpp
    physics_tests.cpp
    scriptengine_tests.cpp
    renderer_tests.cpp
    audio_tests.cpp
    event_input_tests.cpp
    importer_tests.cpp
    integration_tests.cpp
    asset_manager_tests.cpp
    primitive_tests.cpp
    material_tests.cpp
    hierarchy_tests.cpp
    serialization_tests.cpp
    event_tests.cpp
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

target_include_directories(engine_tests_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(engine_tests_integration
    CONFIGURATION_VAR CMAKE_BUILD_TYPE
    DISCOVERY_MODE POST_BUILD
    DISCOVERY_TIMEOUT 60
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    ENVIRONMENT "ENGINE_ROOT=${CMAKE_SOURCE_DIR}"
    PROPERTIES LABELS "Integration"
)