cmake_minimum_required(VERSION 3.31)
project(ChainedDecos)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(raylib CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)

if (NOT raylib_FOUND)
    message(STATUS "Raylib not found, fetching from GitHub...")
    FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG        4.5.0
    )
    FetchContent_MakeAvailable(raylib)
endif()

if (NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching from GitHub...")
    FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        v3.12.0
    )
    FetchContent_MakeAvailable(json)
endif()

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Window/Window.cpp
        src/Window/Window.h
        src/Player/Player.cpp
        src/Player/Player.h
        src/Model/Model.cpp
        src/Model/Model.h
        src/Model/ModelInstance.h
        src/Model/ModelInstance.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib nlohmann_json)
