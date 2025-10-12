# Test configuration
cmake_minimum_required(VERSION 4.1)

set(UNIT_TEST_SOURCES
    main.cpp
    engine/PhysicsComponentTest.cpp
    engine/CollisionSystemTest.cpp
)

set(INTEGRATION_TEST_SOURCES
    game/ParkourMapGeneratorTest.cpp
    integration/GameIntegrationTest.cpp
)

# Create unit test executable (no raylib dependencies)
add_executable(ChainedDecosUnitTests ${UNIT_TEST_SOURCES})

# Link unit tests - no need for additional libraries since tests use mocks
target_link_libraries(ChainedDecosUnitTests
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
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

# Force static linking on Windows to avoid DLL issues
if(WIN32)
    # Add static linking flags for Windows - unit tests
    set_target_properties(ChainedDecosUnitTests PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
        CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )

    # Add static linking flags for Windows - integration tests
    set_target_properties(ChainedDecosIntegrationTests PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
        CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )

    # Add compile definitions to force static linking - unit tests
    target_compile_definitions(ChainedDecosUnitTests PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=0
        GTEST_CREATE_SHARED_LIBRARY=0
    )

    # Add compile definitions to force static linking - integration tests
    target_compile_definitions(ChainedDecosIntegrationTests PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=0
        GTEST_CREATE_SHARED_LIBRARY=0
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

# Performance benchmarks - disabled for now
# if(BUILD_BENCHMARKS)
#     add_subdirectory(benchmarks)
# endif()
