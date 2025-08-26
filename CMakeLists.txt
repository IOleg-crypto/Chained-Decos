cmake_minimum_required(VERSION 3.20)
project(ChainedDecos VERSION 1.0.0 LANGUAGES CXX)


if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# Options to speed up configuration
option(FAST_CONFIG "Reduce CMake configure time" ON)
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_MAP_EDITOR "Build MapEditor tools" OFF)

if(FAST_CONFIG)
  # Use local cache for dependencies and avoid update checks each configure
  set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.deps" CACHE PATH "Deps cache")
  set(FETCHCONTENT_QUIET ON)
  set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG -D_DEBUG -Wall ")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG -D_DEBUG -Wall")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /DDEBUG /D_DEBUG")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W2 /DDEBUG /D_DEBUG")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG")
    endif()
endif()

include(FetchContent)

set(RAYLIB_VERSION "5.5")
set(NLOHMANN_JSON_VERSION "3.12.0")
set(GOOGLETEST_COMMIT "03597a01ee50ed33e9dfd640b249b4be3799d395")


find_package(raylib ${RAYLIB_VERSION} CONFIG QUIET)
if(NOT raylib_FOUND)
    message(STATUS "Raylib ${RAYLIB_VERSION} not found, fetching ZIP...")
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

# nlohmann_json
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching ZIP...")
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/tags/v${NLOHMANN_JSON_VERSION}.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

if(BUILD_TESTS)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GTest not found, downloading ZIP...")
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/${GOOGLETEST_COMMIT}.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        # Force shared CRT on Windows
        if(WIN32)
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        endif()
        # Disable installation of GTest
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

# ImGui library
add_library(imguilib STATIC
    include/rlImGui/imgui_impl_raylib.h
    include/rlImGui/rlImGui.cpp
    include/rlImGui/rlImGui.h
    include/imgui/imgui.cpp
    include/imgui/imgui.h
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_internal.h
    include/imgui/imstb_rectpack.h
    include/imgui/imstb_truetype.h
    include/imgui/misc/cpp/imgui_stdlib.h
    include/imgui/misc/cpp/imgui_stdlib.cpp
    include/raygui/src/raygui.h
)
target_include_directories(imguilib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rlImGui
    $<TARGET_PROPERTY:raylib,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(imguilib PRIVATE raylib)

# Global definitions
add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Global include directory for all subprojects
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add all subprojects
add_subdirectory(src/Engine)
add_subdirectory(src/Player)
add_subdirectory(src/Model)
add_subdirectory(src/Menu)
add_subdirectory(src/Input)
add_subdirectory(src/Map)
add_subdirectory(src/CameraController)
add_subdirectory(src/Color)
add_subdirectory(src/World)
add_subdirectory(src/Collision)
add_subdirectory(src/Render)

# Tests only when enabled
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# MapEditor optional
if(BUILD_MAP_EDITOR)
    add_subdirectory(src/MapEditor)
endif()

# Main executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Include directories for the main executable
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries for the main executable
target_link_libraries(${PROJECT_NAME} PRIVATE
    engine
    player
    model
    menu
    input
    mapmod
    cameracontroller
    color
    world
    collision
    render
    raylib
    nlohmann_json
    imguilib
)
