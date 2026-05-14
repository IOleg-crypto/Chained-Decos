cmake_minimum_required(VERSION 3.31)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Windows SDK Version (must be set before project())
if(WIN32)
    set(WINDOWS_SDK_VERSION "10.0" CACHE STRING "Windows SDK Version")
    set(CMAKE_SYSTEM_VERSION ${WINDOWS_SDK_VERSION})
endif()

project(ChainedDecos LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Standardize output directories for all targets (Set BEFORE including dependencies)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json")
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)


# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(CH_CI "Set to ON for CI environments" OFF)

if(CH_CI)
    add_compile_definitions(CH_CI)
endif()

# Global library definitions
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Warning Settings
option(DISABLE_ALL_WARNINGS "Disable all compiler warnings" OFF)
option(ENABLE_WARNINGS "Enable compiler warnings" OFF)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

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

# === BUILD OPTIMIZATION: Prevent unnecessary recompilation ===
# 1. Don't regenerate build files if nothing changed
set(CMAKE_SUPPRESS_REGENERATION OFF)

# 2. Incremental linking (faster linking)
if(MSVC)
    add_link_options(/INCREMENTAL)
endif()

# 3. Parallel build must be controlled by the build tool (cmake --build --parallel)
# Never pass -j as a compiler flag, because compilers like gcc treat it as invalid.

# Dependencies
include(cmake/CompilerSettings.cmake)
include(cmake/Dependencies.cmake)
include(cmake/ProjectHelpers.cmake)

if(EXISTS "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
    set(CH_PYTHON_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe" CACHE FILEPATH "Python interpreter used for build helper scripts" FORCE)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python")
    set(CH_PYTHON_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python" CACHE FILEPATH "Python interpreter used for build helper scripts" FORCE)
else()
    find_program(CH_PYTHON_EXECUTABLE NAMES python3 python REQUIRED)
endif()

chained_generate_build_preset_header()

set(CH_ACTIVE_GAME "chaineddecos" CACHE STRING "Active game project to build")
set_property(CACHE CH_ACTIVE_GAME PROPERTY STRINGS chaineddecos testproject)

# Useful definitions
if(UNIX AND NOT APPLE)
    add_compile_definitions(_GNU_SOURCE)
endif()

if(MINGW)
    add_compile_options(-Wa,-mbig-obj)
endif()

add_subdirectory(engine)

# ── Build Game Projects ──────────────────────────────────────────────────────
# Automatically find and include all projects in the game/ directory.
file(GLOB GAME_PROJECT_LIST RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/game" "game/*")
foreach(GAME_PROJECT_DIR ${GAME_PROJECT_LIST})
    set(GAME_PATH "${CMAKE_CURRENT_SOURCE_DIR}/game/${GAME_PROJECT_DIR}")
    if(IS_DIRECTORY "${GAME_PATH}" AND EXISTS "${GAME_PATH}/CMakeLists.txt")
        # If CH_ACTIVE_GAME is defined, only add the specific one to save config time
        if(NOT CH_ACTIVE_GAME OR CH_ACTIVE_GAME STREQUAL GAME_PROJECT_DIR)
            add_subdirectory("game/${GAME_PROJECT_DIR}")
        endif()
    endif()
endforeach()

add_subdirectory(editor)
add_subdirectory(runtime)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Post-Build Asset Sync via Python Script ---
add_custom_target(SyncAssets ALL
    COMMAND "${CH_PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/build_scripts/build_managed.py" sync-resources --root "${CMAKE_SOURCE_DIR}" --bin "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    COMMENT "Syncing engine assets via Python script..."
)
