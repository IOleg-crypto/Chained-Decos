cmake_minimum_required(VERSION 3.31)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(ChainedDecos LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json")


# Options
option(BUILD_TESTS "Build unit tests" ON)


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

# Legacy support (optional, commented out)
# add_subdirectory(v1_legacy)
