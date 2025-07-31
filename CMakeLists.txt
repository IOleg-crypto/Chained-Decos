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

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found, downloading via FetchContent")

    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    if(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(googletest)
endif()


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
)

target_include_directories(imguilib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rlImGui
        $<TARGET_PROPERTY:raylib,INTERFACE_INCLUDE_DIRECTORIES>

)

target_link_libraries(imguilib PRIVATE raylib)

add_subdirectory(tests)

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
        src/Engine/Engine.cpp
        src/Engine/Engine.h
        src/Color/ColorParser.h
        src/Map/MapLoader.cpp
        src/Map/MapLoader.h
        src/CameraController/CameraController.h
        src/CameraController/CameraController.cpp
        src/World/PhysicsData.h
        src/Player/PositionData.h
        src/Input/InputManager.cpp
        src/Input/InputManager.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src

)

target_link_libraries(${PROJECT_NAME}
        PRIVATE raylib nlohmann_json imguilib GTest::gtest_main
)