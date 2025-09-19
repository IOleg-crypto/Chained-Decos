cmake_minimum_required(VERSION 3.20)
project(ChainedDecos VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(FAST_CONFIG "Reduce CMake configure time" ON)
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_MAP_EDITOR "Build MapEditor tools" OFF)

if(FAST_CONFIG)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.deps" CACHE PATH "Deps cache")
    set(FETCHCONTENT_QUIET ON)
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)
endif()

include(FetchContent)

# --- Fetch external dependencies ---
set(RAYLIB_VERSION "5.5")
set(NLOHMANN_JSON_VERSION "3.12.0")

# Raylib
find_package(raylib ${RAYLIB_VERSION} CONFIG QUIET)

if(NOT raylib_FOUND)
    message(STATUS "Raylib ${RAYLIB_VERSION} not found, fetching...")
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

# nlohmann_json
find_package(nlohmann_json CONFIG QUIET)

if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching...")
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/tags/v${NLOHMANN_JSON_VERSION}.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(imguilib STATIC
    include/rlImGui/rlImGui.cpp
    include/rlImGui/rlImGui.h
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imguilib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rlImGui
)
target_link_libraries(imguilib PRIVATE raylib)

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()
add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

add_subdirectory(src/Engine)
add_subdirectory(src/Game)

add_executable(ChainedDecos src/main.cpp)
target_link_libraries(ChainedDecos PRIVATE
    ChainedDecosGame
    imguilib
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(BUILD_MAP_EDITOR)
    add_subdirectory(src/Game/MapEditor)
endif()
