# Core Logic Tests (No Engine Boot, Faster, CI-Friendly)
add_executable(CoreTests
    uuid_tests.cpp
    event_tests.cpp
)

# --- Split Engine Integration Tests ---

# 1. Asset & Importer Tests
add_executable(AssetTests
    asset_manager_tests.cpp
    importer_tests.cpp
    test_environment.cpp
)

# 2. Scene & ECS Tests
add_executable(SceneTests
    scene_tests.cpp
    hierarchy_tests.cpp
    uuid_tests.cpp
    test_environment.cpp
)

# 3. System Tests (Physics, Audio, Scripting)
add_executable(SystemTests
    physics_tests.cpp
    audio_tests.cpp
    scriptengine_tests.cpp
    event_input_tests.cpp
    test_environment.cpp
)

# 4. Rendering & Material Tests
add_executable(GraphicsTests
    renderer_tests.cpp
    material_tests.cpp
    example_render_test.cpp
    primitive_tests.cpp
    test_environment.cpp
)

# 5. Serialization Tests
add_executable(SerializationTests
    serialization_tests.cpp
    scene_serialization_tests.cpp
    test_environment.cpp
)

# 6. Integration/Main Path Tests
add_executable(IntegrationTests
    integration_tests.cpp
    bvh_tests.cpp
    test_environment.cpp
)

set(ALL_TEST_TARGETS CoreTests AssetTests SceneTests SystemTests GraphicsTests SerializationTests IntegrationTests)

# Linkage & Configuration
foreach(test_target ${ALL_TEST_TARGETS})
    target_link_libraries(${test_target} PRIVATE engine GTest::gtest_main GTest::gmock)
    if(NOT test_target STREQUAL "CoreTests")
        target_link_libraries(${test_target} PRIVATE editor_core)
        target_include_directories(${test_target} PRIVATE ${CMAKE_SOURCE_DIR}/editor)
    endif()
    
    target_include_directories(${test_target} PRIVATE ${CMAKE_SOURCE_DIR})
    apply_engine_optimizations(${test_target})

    include(GoogleTest)
    gtest_discover_tests(${test_target}
        CONFIGURATION_VAR CMAKE_BUILD_TYPE
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        PROPERTIES LABELS "Engine;${test_target}"
    )
    
    install(TARGETS ${test_target} RUNTIME DESTINATION bin)
endforeach()
