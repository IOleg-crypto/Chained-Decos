cmake_minimum_required(VERSION 3.31)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Enforce static CRT (/MT) globally — must be set before project() or early
# so that third-party add_subdirectory calls don't override it.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Windows SDK Version (must be set before project())
if(WIN32)
    set(WINDOWS_SDK_VERSION "10.0" CACHE STRING "Windows SDK Version")
    set(CMAKE_SYSTEM_VERSION ${WINDOWS_SDK_VERSION})
endif()

project(ChainedDecos LANGUAGES CXX C)



# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(CH_CI "Set to ON for CI environments" OFF)

if(CH_CI)
    add_compile_definitions(CH_CI)
endif()

# Global library definitions
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Enable logging only in Debug and RelWithDebInfo configurations
add_compile_definitions($<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:CH_ENABLE_LOGGING>)


# Optional: ccache support for faster rebuilds
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

# === BUILD OPTIMIZATION ===
set(CMAKE_SUPPRESS_REGENERATION OFF)
if(MSVC)
    add_link_options(/INCREMENTAL)
endif()

# Dependencies
include(cmake/CompilerSettings.cmake)
include(cmake/Dependencies.cmake)
include(cmake/ProjectHelpers.cmake)

# Find Python interpreter
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CH_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}" CACHE FILEPATH "Python interpreter used for build helper scripts" FORCE)

set(CH_ACTIVE_GAME "chaineddecos" CACHE STRING "Active game project to build")
set_property(CACHE CH_ACTIVE_GAME PROPERTY STRINGS chaineddecos testproject)

if(UNIX AND NOT APPLE)
    add_compile_definitions(_GNU_SOURCE)
endif()

if(MINGW)
    add_compile_options(-Wa,-mbig-obj)
endif()


add_subdirectory(engine)

# ── Build Game Projects ──────────────────────────────────────────────────────
file(GLOB GAME_PROJECT_LIST RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/game" "game/*")
foreach(GAME_PROJECT_DIR ${GAME_PROJECT_LIST})
    set(GAME_PATH "${CMAKE_CURRENT_SOURCE_DIR}/game/${GAME_PROJECT_DIR}")
    if(IS_DIRECTORY "${GAME_PATH}" AND EXISTS "${GAME_PATH}/CMakeLists.txt")
        if(NOT CH_ACTIVE_GAME OR CH_ACTIVE_GAME STREQUAL GAME_PROJECT_DIR)
            add_subdirectory("game/${GAME_PROJECT_DIR}")
        endif()
    endif()
endforeach()

add_subdirectory(editor)

enable_testing()
add_subdirectory(tests)

# ── Resource Sync Macros & Targets ───────────────────────────────────────────
macro(ch_add_resource_sync TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CH_PYTHON_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/tools/sync_resources.py"
                sync-resources
                --root "${CMAKE_SOURCE_DIR}"
                --bin  "$<TARGET_FILE_DIR:${TARGET}>"
        COMMENT "Syncing resources for ${TARGET} ($<CONFIG>)"
        VERBATIM
    )
endmacro()

add_custom_target(sync-resources
    COMMAND ${CH_PYTHON_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tools/sync_resources.py"
            sync-resources
            --root "${CMAKE_SOURCE_DIR}"
            --bin  "${CMAKE_BINARY_DIR}/bin"
            --all-configs
    COMMENT "Syncing resources to all configs"
    VERBATIM
)