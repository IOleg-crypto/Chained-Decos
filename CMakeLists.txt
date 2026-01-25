cmake_minimum_required(VERSION 3.31)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(ChainedDecos LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)

# Global Include Paths - Removed in favor of target_include_directories
# include_directories(${CMAKE_SOURCE_DIR})

# Dependencies
include(cmake/CompilerSettings.cmake)
include(cmake/Dependencies.cmake)

# Useful definitions
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
add_compile_definitions(PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}")

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
