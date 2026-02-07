cmake_minimum_required(VERSION 3.31)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(ChainedDecos LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json")
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)


# Options
option(BUILD_TESTS "Build unit tests" ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static linkage" FORCE)
option(CH_CI "Set to ON for CI environments" OFF)

if(CH_CI)
    add_compile_definitions(CH_CI)
endif()

# Optional: ccache support for faster rebuilds
# Install ccache: choco install ccache (Windows) or apt install ccache (Linux)
option(USE_CCACHE "Use ccache if available" ON)
if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    else()
        message(STATUS "ccache not found - install with: choco install ccache")
    endif()
endif()


# Dependencies
include(cmake/CompilerSettings.cmake)
include(cmake/Dependencies.cmake)
include(cmake/ProjectHelpers.cmake)

# Useful definitions
if(UNIX AND NOT APPLE)
    add_compile_definitions(_GNU_SOURCE)
endif()

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
add_compile_definitions(PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}")

# Standardize output directories for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(engine)
add_subdirectory(game/chaineddecos)
add_subdirectory(editor)
add_subdirectory(runtime)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

