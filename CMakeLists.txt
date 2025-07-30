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
        src/Engine/Engine.cpp
        src/Engine/Engine.h
        src/Color/ColorParser.h
        src/Map/MapLoader.cpp
        src/Map/MapLoader.h


)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rlImGui
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/examples/libs/glfw/include/GLFW
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE raylib nlohmann_json
)
