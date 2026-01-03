cmake_minimum_required(VERSION 3.25)
project(CHEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================================
# Build Configuration Options
# ============================================================================
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EDITOR_ENGINE "Build Editor tools" ON)
option(ENABLE_PROFILING "Enable profiling support" OFF)
option(ENABLE_WARNINGS "Enable strict compiler warnings" OFF)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(ENABLE_UNITY_BUILD "Enable unity build for faster compilation" ON)

# Platform-specific options
option(ENABLE_OPENGL_DEBUG "Enable OpenGL debug context" OFF)
option(ENABLE_AUDIO "Enable audio support" ON)

# Development options
option(ENABLE_DEBUG_INFO "Enable debug information" ON)
option(ENABLE_SANITIZERS "Enable sanitizers for debugging" OFF)

# Force static linking
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Set default install prefix
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install prefix" FORCE)

# ============================================================================
# Include Modular CMake Files
# ============================================================================
include(cmake/CompilerSettings.cmake)
include(cmake/Dependencies.cmake)

# Unity build configuration
if(ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD OFF)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)
endif()

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
add_compile_definitions(PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}")

# Copy resources to build directory for runtime access
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# ============================================================================
# Add Engine Modules
# ============================================================================
add_subdirectory(events)
add_subdirectory(core)
add_subdirectory(components)
add_subdirectory(scene)
add_subdirectory(project)
add_subdirectory(runtime)

if(BUILD_TESTS)
    add_subdirectory(tests ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()

if(BUILD_EDITOR_ENGINE)
    add_subdirectory(editor)
endif()

# ============================================================================
# Install Configuration
# ============================================================================
install(FILES game.cfg
    DESTINATION share/CHEngine
)

# Package configuration
if(NOT FAST_CONFIG)
    set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
endif()
