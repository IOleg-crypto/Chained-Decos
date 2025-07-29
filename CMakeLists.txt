cmake_minimum_required(VERSION 3.31)
project(ChainedDecos)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)

# Raylib
find_package(raylib CONFIG QUIET)
if (NOT raylib_FOUND)
    message(STATUS "Raylib not found, fetching from GitHub...")
    FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG        5.5
    )
    FetchContent_MakeAvailable(raylib)
endif()

# nlohmann_json
find_package(nlohmann_json CONFIG QUIET)
if (NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching from GitHub...")
    FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        v3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Player/Player.cpp
        src/Player/Player.h
        src/Model/Model.cpp
        src/Model/Model.h
        src/Model/ModelInstance.cpp
        src/Model/ModelInstance.h
        src/Menu/Menu.cpp
        src/Menu/Menu.h
        src/rlImGui/imgui_impl_raylib.h
        src/rlImGui/rlImGui.cpp
        src/rlImGui/rlImGui.h
        src/imgui/imgui.cpp
        src/imgui/imgui.h
        src/imgui/imgui_draw.cpp
        src/imgui/imgui_demo.cpp
        src/imgui/imgui_tables.cpp
        src/imgui/imgui_widgets.cpp
        src/imgui/imgui_internal.h
        src/imgui/imstb_rectpack.h
        src/imgui/imstb_truetype.h
        src/Engine/Engine.cpp
        src/Engine/Engine.h
        src/Color/ColorParser.h


)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/imgui ${CMAKE_SOURCE_DIR}/src/rlImGui)
target_link_libraries(${PROJECT_NAME}
        PRIVATE raylib nlohmann_json
)
