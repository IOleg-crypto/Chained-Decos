cmake_minimum_required(VERSION 3.31)
project(ChainedDecos)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ------------------------
# Fetch raylib
# ------------------------
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

# ------------------------
# Fetch nlohmann_json
# ------------------------
find_package(nlohmann_json CONFIG QUIET)

if (NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching from GitHub...")
    FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        v3.12.0
    )
    FetchContent_MakeAvailable(json)
endif()

# ------------------------
# Fetch ImGui manually into src/ImGui
# ------------------------
message(STATUS "imgui not found, fetching into src/ImGui...")
FetchContent_Declare(
        imgui_lib
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.92.1
        SOURCE_DIR     ${CMAKE_SOURCE_DIR}/src/ImGui
)
FetchContent_MakeAvailable(imgui_lib)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/ImGui)

# ------------------------
# Executable and sources
# ------------------------
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Window/Window.cpp
        src/Window/Window.h
        src/Player/Player.cpp
        src/Player/Player.h
        src/Model/Model.cpp
        src/Model/Model.h
        src/Model/ModelInstance.cpp
        src/Model/ModelInstance.h
        src/Menu/Menu.cpp
        src/Menu/Menu.h

        # ImGui sources
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp

)

# ------------------------
# Includes and linking
# ------------------------
target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib nlohmann_json)
