# Test configuration
set(TEST_SOURCES
    main.cpp
    engine/PhysicsComponentTest.cpp
    engine/CollisionSystemTest.cpp
)

# Create test executable
add_executable(ChainedDecosTests ${TEST_SOURCES})

# Link with main libraries
target_link_libraries(ChainedDecosTests
    PRIVATE
        ChainedDecosEngine
        # Only link specific game components needed for tests, not full ChainedDecosGame
        # ChainedDecosGame  # Commented out to avoid GLFW dependency in tests
        GTest::gtest_main
        GTest::gmock_main
)

# Include directories
target_include_directories(ChainedDecosTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# Test properties
set_target_properties(ChainedDecosTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Force static linking on Windows to avoid DLL issues
if(WIN32)
    # Add static linking flags for Windows
    set_target_properties(ChainedDecosTests PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
        CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )

    # Add compile definitions to force static linking
    target_compile_definitions(ChainedDecosTests PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=0
        GTEST_CREATE_SHARED_LIBRARY=0
    )

    # Add winmm library for Windows timer functions (needed by raylib)
    target_link_libraries(ChainedDecosTests PRIVATE winmm)
endif()

# Enable test discovery
include(GoogleTest)

# Use a more robust working directory configuration
if(DEFINED ENV{CI} OR DEFINED ENV{GITHUB_ACTIONS})
    # In CI environments, use a more specific path
    set(TEST_WORKING_DIR "${CMAKE_BINARY_DIR}/bin/tests")
else()
    # In local development, use the target file directory
    set(TEST_WORKING_DIR "$<TARGET_FILE_DIR:ChainedDecosTests>")
endif()

gtest_discover_tests(ChainedDecosTests
    WORKING_DIRECTORY "${TEST_WORKING_DIR}"
)

# Additional custom command to ensure tests run from correct directory
add_custom_target(run_tests_with_correct_dir
    COMMAND ${CMAKE_COMMAND} -E echo "Running tests from: $<TARGET_FILE_DIR:ChainedDecosTests>"
    COMMAND cd $<TARGET_FILE_DIR:ChainedDecosTests> && $<TARGET_FILE:ChainedDecosTests>
    DEPENDS ChainedDecosTests
    COMMENT "Running tests from correct working directory"
)


# Custom test targets
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Unit" --output-on-failure
    DEPENDS ChainedDecosTests
    COMMENT "Running unit tests"
)

add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Integration" --output-on-failure
    DEPENDS ChainedDecosTests
    COMMENT "Running integration tests"
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ChainedDecosTests
    COMMENT "Running all tests"
)

# Performance benchmarks - disabled for now
# if(BUILD_BENCHMARKS)
#     add_subdirectory(benchmarks)
# endif()
