add_executable(EngineTests
    uuid_tests.cpp
    primitive_tests.cpp
    serialization_tests.cpp
    scriptengine_tests.cpp
    scene_tests.cpp
    physics_tests.cpp
    hierarchy_tests.cpp
    event_input_tests.cpp
    asset_manager_tests.cpp
    integration_tests.cpp
    importer_tests.cpp
    renderer_tests.cpp
    scene_serialization_tests.cpp
    material_tests.cpp
    audio_tests.cpp
    test_environment.cpp
)

target_link_libraries(EngineTests PRIVATE engine editor_core GTest::gtest_main)
apply_engine_optimizations(EngineTests)
target_include_directories(EngineTests PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/editor
)

# Use centralized engine resources
chained_add_engine_resources_copy()
add_dependencies(EngineTests EngineResources)

include(GoogleTest)
gtest_discover_tests(EngineTests 
    CONFIGURATION_VAR CMAKE_BUILD_TYPE
    DISCOVERY_MODE PRE_TEST
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    PROPERTIES LABELS "Engine;Core"
)

# Installation
install(TARGETS EngineTests
    RUNTIME DESTINATION bin
)
