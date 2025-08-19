cmake_minimum_required(VERSION 3.20)
project(ChainedDecos VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG -D_DEBUG -Wall -Wextra")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG -D_DEBUG -Wall -Wextra")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 /DDEBUG /D_DEBUG")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W4 /DDEBUG /D_DEBUG")
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

# Include FetchContent for dependencies
include(FetchContent)

set(RAYLIB_VERSION "5.5")
set(NLOHMANN_JSON_VERSION "3.12.0")  
set(GOOGLETEST_COMMIT "03597a01ee50ed33e9dfd640b249b4be3799d395")

# Raylib
find_package(raylib ${RAYLIB_VERSION} CONFIG QUIET)
if(NOT raylib_FOUND)
    message(STATUS "Raylib ${RAYLIB_VERSION} not found, fetching from GitHub...")
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        ${RAYLIB_VERSION}
    )
    # Disable raylib examples and tests
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

# nlohmann_json
find_package(nlohmann_json CONFIG QUIET)  # Removed version to avoid format issues
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching from GitHub...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v${NLOHMANN_JSON_VERSION}  # Added 'v' prefix here instead
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# GTest
find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found, downloading via FetchContent")
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
add_subdirectory(tests)
add_subdirectory(src/MapEditor)

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

# Install rules (optional)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Copy resources to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying resources to build directory"
)