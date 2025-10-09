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

# Add winmm library for Windows timer functions (needed by raylib)
if(WIN32)
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

# Copy required DLLs for Windows
if(WIN32)
    # Find all DLL files in multiple possible locations for different build environments
    file(GLOB_RECURSE DLL_FILES
        "${CMAKE_BINARY_DIR}/bin/*.dll"
        "${CMAKE_BINARY_DIR}/*.dll"
        "${CMAKE_SOURCE_DIR}/.deps/*/bin/*.dll"
        "${CMAKE_SOURCE_DIR}/.deps/*/*.dll"
        # CI environment paths
        "D:/a/*/build/bin/*.dll"
        "D:/a/*/*.dll"
    )

    # Also copy specific essential DLLs that might be in subdirectories
    set(ESSENTIAL_DLLS
        "raylib.dll"
        "glfw3.dll"
        "openal32.dll"
        "winmm.dll"
    )

    foreach(DLL_FILE ${DLL_FILES})
        get_filename_component(DLL_NAME ${DLL_FILE} NAME)
        add_custom_command(TARGET ChainedDecosTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL_FILE}
            $<TARGET_FILE_DIR:ChainedDecosTests>/${DLL_NAME}
            COMMENT "Copying ${DLL_NAME} to test directory"
        )
    endforeach()

    # Additional step to ensure essential DLLs are copied from common locations
    foreach(ESSENTIAL_DLL ${ESSENTIAL_DLLS})
        if(EXISTS "${CMAKE_BINARY_DIR}/bin/${ESSENTIAL_DLL}")
            add_custom_command(TARGET ChainedDecosTests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/bin/${ESSENTIAL_DLL}"
                $<TARGET_FILE_DIR:ChainedDecosTests>/${ESSENTIAL_DLL}
                COMMENT "Ensuring ${ESSENTIAL_DLL} is available"
            )
        endif()
    endforeach()

    # Also try to copy from raylib installation directory
    if(EXISTS "C:/Program Files/raylib/bin/${ESSENTIAL_DLLS}")
        foreach(ESSENTIAL_DLL ${ESSENTIAL_DLLS})
            add_custom_command(TARGET ChainedDecosTests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "C:/Program Files/raylib/bin/${ESSENTIAL_DLL}"
                $<TARGET_FILE_DIR:ChainedDecosTests>/${ESSENTIAL_DLL}
                COMMENT "Copying ${ESSENTIAL_DLL} from raylib installation"
            )
        endforeach()
    endif()
endif()

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
