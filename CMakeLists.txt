cmake_minimum_required(VERSION 3.31)
project(Game)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(raylib CONFIG QUIET)

if (NOT raylib_FOUND)
    message(STATUS "Raylib not found, fetching from GitHub...")
    FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG        4.5.0
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(Game src/main.cpp
        src/Window/Window.cpp
        src/Window/Window.h)

target_link_libraries(Game PRIVATE raylib)
