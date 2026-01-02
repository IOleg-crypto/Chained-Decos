# Test configuration
cmake_minimum_required(VERSION 3.29)

set(UNIT_TEST_SOURCES
    engine/PhysicsComponentTest.cpp
    engine/CollisionSystemTest.cpp
    engine/SettingsManagerTest.cpp
)

set(INTEGRATION_TEST_SOURCES
    integration/EditorInputTest.cpp
)

# Create unit test executable (no raylib dependencies)
add_executable(ChainedEngineUnitTests ${UNIT_TEST_SOURCES})

# Link unit tests
target_link_libraries(ChainedEngineUnitTests
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        core
)

# Create integration test executable (with full game dependencies)
add_executable(ChainedEngineIntegrationTests ${INTEGRATION_TEST_SOURCES})

# Link integration tests with full game
target_link_libraries(ChainedEngineIntegrationTests
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        RuntimeLib
)

# Include directories for unit tests
target_include_directories(ChainedEngineUnitTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
)

# Include directories for integration tests
target_include_directories(ChainedEngineIntegrationTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include/raylib/src
        ${CMAKE_SOURCE_DIR}/include/entt/src
)

# Scene/Entity System Test (standalone validation)
add_executable(SceneEntityTest scene/SceneEntityTest.cpp)
target_link_libraries(SceneEntityTest
    PRIVATE
        scene_core
        ecs
        raylib
        core
)
target_include_directories(SceneEntityTest
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

# Test properties for unit tests
set_target_properties(ChainedEngineUnitTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Test properties for integration tests
set_target_properties(ChainedEngineIntegrationTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Ensure consistent CRT settings on Windows
if(WIN32)
    # Use appropriate CRT based on build type - unit tests
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(ChainedEngineUnitTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL"
            CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL"
        )
    else()
        set_target_properties(ChainedEngineUnitTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
            CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
        )
    endif()

    # Use appropriate CRT based on build type - integration tests
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(ChainedEngineIntegrationTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL"
            CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL"
        )
    else()
        set_target_properties(ChainedEngineIntegrationTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
            CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
        )
    endif()


    # Add compile definitions for dynamic linking - unit tests
    target_compile_definitions(ChainedEngineUnitTests PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=1
        GTEST_CREATE_SHARED_LIBRARY=1
    )

    # Add compile definitions for dynamic linking - integration tests
    target_compile_definitions(ChainedEngineIntegrationTests PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=1
        GTEST_CREATE_SHARED_LIBRARY=1
    )

    # Add winmm library for Windows timer functions (needed by some system functions)
    target_link_libraries(ChainedEngineUnitTests PRIVATE winmm)
endif()

# Custom test targets that run Google Test executables directly

# Main test target - runs unit tests
add_custom_target(test
    COMMAND $<TARGET_FILE:ChainedEngineUnitTests>
    DEPENDS ChainedEngineUnitTests
    COMMENT "Running unit tests"
)

# Run unit tests specifically
add_custom_target(run_unit_tests
    COMMAND $<TARGET_FILE:ChainedEngineUnitTests>
    DEPENDS ChainedEngineUnitTests
    COMMENT "Running unit tests"
)

# Run integration tests specifically
add_custom_target(run_integration_tests
    COMMAND $<TARGET_FILE:ChainedEngineIntegrationTests>
    DEPENDS ChainedEngineIntegrationTests
    COMMENT "Running integration tests"
)

# Run all tests (both unit and integration)
add_custom_target(run_all_tests
    COMMAND $<TARGET_FILE:ChainedEngineUnitTests>
    COMMAND $<TARGET_FILE:ChainedEngineIntegrationTests>
    DEPENDS ChainedEngineUnitTests ChainedEngineIntegrationTests
    COMMENT "Running all tests (unit and integration)"
)



