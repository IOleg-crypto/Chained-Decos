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
        ChainedDecosGame
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

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(ChainedDecosTests)

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
