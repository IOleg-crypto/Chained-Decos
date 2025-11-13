# Test configuration
cmake_minimum_required(VERSION 3.25)

set(UNIT_TEST_SOURCES
    engine/PhysicsComponentTest.cpp
    engine/CollisionSystemTest.cpp
    engine/ConfigManagerTest.cpp
)

set(INTEGRATION_TEST_SOURCES
    game/PlayerTest.cpp
    integration/GameIntegrationTest.cpp
    integration/SkyboxTest.cpp
    engine/SettingsManagerTest.cpp
)

# Create unit test executable (no raylib dependencies)
add_executable(ChainedDecosUnitTests ${UNIT_TEST_SOURCES})

# Link unit tests - need config library for ConfigManager tests
target_link_libraries(ChainedDecosUnitTests
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        config
)

# Create integration test executable (with full game dependencies)
add_executable(ChainedDecosIntegrationTests ${INTEGRATION_TEST_SOURCES})

# Link integration tests with full game
target_link_libraries(ChainedDecosIntegrationTests
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        ChainedDecosGame
)

# Include directories for unit tests
target_include_directories(ChainedDecosUnitTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# Include directories for integration tests
target_include_directories(ChainedDecosIntegrationTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# Test properties for unit tests
set_target_properties(ChainedDecosUnitTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Test properties for integration tests
set_target_properties(ChainedDecosIntegrationTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Ensure consistent CRT settings on Windows
if(WIN32)
    # Use appropriate CRT based on build type - unit tests
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(ChainedDecosUnitTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL"
            CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL"
        )
    else()
        set_target_properties(ChainedDecosUnitTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
            CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
        )
    endif()

    # Use appropriate CRT based on build type - integration tests
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(ChainedDecosIntegrationTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL"
            CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL"
        )
    else()
        set_target_properties(ChainedDecosIntegrationTests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
            CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
        )
    endif()

    # Force consistent debug levels for both test targets
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(ChainedDecosUnitTests PRIVATE _DEBUG)
        target_compile_definitions(ChainedDecosIntegrationTests PRIVATE _DEBUG)
    else()
        target_compile_definitions(ChainedDecosUnitTests PRIVATE NDEBUG)
        target_compile_definitions(ChainedDecosIntegrationTests PRIVATE NDEBUG)
    endif()

    # Add compile definitions for dynamic linking - unit tests
    target_compile_definitions(ChainedDecosUnitTests PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=1
        GTEST_CREATE_SHARED_LIBRARY=1
    )

    # Add compile definitions for dynamic linking - integration tests
    target_compile_definitions(ChainedDecosIntegrationTests PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=1
        GTEST_CREATE_SHARED_LIBRARY=1
    )

    # Add winmm library for Windows timer functions (needed by some system functions)
    target_link_libraries(ChainedDecosUnitTests PRIVATE winmm)
endif()

# Custom test targets that run Google Test executables directly

# Main test target - runs unit tests
add_custom_target(test
    COMMAND $<TARGET_FILE:ChainedDecosUnitTests>
    DEPENDS ChainedDecosUnitTests
    COMMENT "Running unit tests"
)

# Run unit tests specifically
add_custom_target(run_unit_tests
    COMMAND $<TARGET_FILE:ChainedDecosUnitTests>
    DEPENDS ChainedDecosUnitTests
    COMMENT "Running unit tests"
)

# Run integration tests specifically
add_custom_target(run_integration_tests
    COMMAND $<TARGET_FILE:ChainedDecosIntegrationTests>
    DEPENDS ChainedDecosIntegrationTests
    COMMENT "Running integration tests"
)

# Run all tests (both unit and integration)
add_custom_target(run_all_tests
    COMMAND $<TARGET_FILE:ChainedDecosUnitTests>
    COMMAND $<TARGET_FILE:ChainedDecosIntegrationTests>
    DEPENDS ChainedDecosUnitTests ChainedDecosIntegrationTests
    COMMENT "Running all tests (unit and integration)"
)

