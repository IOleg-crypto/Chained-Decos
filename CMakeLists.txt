cmake_minimum_required(VERSION 3.25)
project(CHEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build configuration options
# Build configuration options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EDITOR_ENGINE "Build Editor tools" ON)
option(ENABLE_PROFILING "Enable profiling support" OFF)
option(ENABLE_WARNINGS "Enable strict compiler warnings" OFF)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(ENABLE_UNITY_BUILD "Enable unity build for faster compilation" ON)

# Platform-specific options
option(ENABLE_OPENGL_DEBUG "Enable OpenGL debug context" OFF)
option(ENABLE_AUDIO "Enable audio support" ON)

# Development options
option(ENABLE_DEBUG_INFO "Enable debug information" ON)
option(ENABLE_SANITIZERS "Enable sanitizers for debugging" OFF)

# Force static linking
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Set default install prefix
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install prefix" FORCE)

# Compiler-specific settings
if(MSVC)
    # MSVC-specific settings
    add_compile_options(
        $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/MT> $<$<CONFIG:Release>:/DNDEBUG>
        /Zi /EHsc
    )

    if(ENABLE_WARNINGS)
        add_compile_options(/W4 /permissive-)
        if(WARNINGS_AS_ERRORS)
            add_compile_options(/WX)
        endif()
    else()
        add_compile_options(/W0)
    endif()

    if(ENABLE_SANITIZERS)
        add_compile_options(/fsanitize=address)
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang settings
    add_compile_options(
        $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-DNDEBUG>
    )

    if(ENABLE_WARNINGS)
        add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-Wmost -Wno-missing-braces -Wno-missing-field-initializers -Wno-attributes)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-Wno-missing-field-initializers -Wno-attributes)
        endif()

        if(WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    else()
        add_compile_options(-w)
    endif()

    if(ENABLE_DEBUG_INFO)
        add_compile_options(-g)
    endif()

    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()

    if(ENABLE_PROFILING)
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()
endif()

# Platform-specific settings
if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7+
endif()


# Global include directories - allow including from project root
include_directories(${CMAKE_SOURCE_DIR})

include(FetchContent)

find_package(Threads REQUIRED)

# Unity build configuration (DISABLED for cleaner builds)
if(ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD OFF)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)
endif()

# --- Load dependencies from submodules ---

# Raylib
if(EXISTS "${CMAKE_SOURCE_DIR}/include/raylib/CMakeLists.txt")
    message(STATUS "Loading Raylib from submodule...")
    
    # Raylib build configuration - static only
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    # Platform-specific raylib options
    if(UNIX AND NOT APPLE)
        set(USE_WAYLAND OFF CACHE BOOL "" FORCE)
        set(PLATFORM Desktop CACHE STRING "" FORCE)
        set(OPENGL_VERSION "3.3" CACHE STRING "" FORCE)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
        set(USE_EXTERNAL_GLFW IF_POSSIBLE CACHE STRING "" FORCE)
    endif()

    add_subdirectory(include/raylib)
    message(STATUS "Raylib loaded from submodule")
else()
    message(FATAL_ERROR "Raylib submodule not found. Run: git submodule update --init --recursive")
endif()

# nlohmann_json
if(EXISTS "${CMAKE_SOURCE_DIR}/include/json/CMakeLists.txt")
    message(STATUS "Loading nlohmann_json from submodule...")
    add_subdirectory(include/json)
    message(STATUS "nlohmann_json loaded from submodule")
else()
    message(FATAL_ERROR "nlohmann_json submodule not found. Run: git submodule update --init --recursive")
endif()

# GoogleTest (for unit tests)
if(BUILD_TESTS)
    if(EXISTS "${CMAKE_SOURCE_DIR}/include/googletest/CMakeLists.txt")
        message(STATUS "Loading GoogleTest from submodule...")
        set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
        
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_compile_definitions(_DEBUG)
        else()
            add_compile_definitions(NDEBUG)
        endif()
        
        add_subdirectory(include/googletest)
        message(STATUS "GoogleTest loaded from submodule")
    else()
        message(WARNING "GoogleTest submodule not found. Tests will be disabled.")
    endif()
endif()

# EnTT (header-only library)
if(EXISTS "${CMAKE_SOURCE_DIR}/include/entt/src")
    message(STATUS "Loading EnTT from submodule...")
    add_library(EnTT INTERFACE)
    target_include_directories(EnTT INTERFACE ${CMAKE_SOURCE_DIR}/include/entt/src)
    add_library(EnTT::EnTT ALIAS EnTT)
    message(STATUS "EnTT loaded from submodule (header-only)")
else()
    message(FATAL_ERROR "EnTT submodule not found. Run: git submodule update --init --recursive")
endif()

set(IMGUI_SOURCES
    include/rlImGui/rlImGui.cpp
    include/rlImGui/rlImGui.h
    include/rlImGui/imgui_impl_raylib.h
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/misc/cpp/imgui_stdlib.cpp
)

add_library(imguilib STATIC ${IMGUI_SOURCES})

target_include_directories(imguilib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rlImGui
)
target_link_libraries(imguilib PRIVATE raylib)

# Define IMGUI math operators before including imgui.h
target_compile_definitions(imguilib PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# Disable unity build for imguilib to avoid GLAD header conflicts
set_target_properties(imguilib PROPERTIES UNITY_BUILD OFF)

# Disable unity build for raylib to avoid GLAD header conflicts
set_target_properties(raylib PROPERTIES UNITY_BUILD OFF)

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
add_compile_definitions(PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}")

# Copy resources to build directory for runtime access
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# Add new modular structure
add_subdirectory(events)
add_subdirectory(core)
add_subdirectory(components)
add_subdirectory(scene)
add_subdirectory(project)
add_subdirectory(runtime)

if(BUILD_TESTS)
    add_subdirectory(tests ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()

if(BUILD_EDITOR_ENGINE)
    add_subdirectory(editor)
endif()



# Native File Dialog (nfd)
if(EXISTS "${CMAKE_SOURCE_DIR}/include/nfd/CMakeLists.txt")
    message(STATUS "Loading nfd from submodule...")
    set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(include/nfd)
    message(STATUS "nfd loaded from submodule")
endif()

# Lua & sol2 integration
message(STATUS "Fetching Lua and sol2...")

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/marovira/lua.git
    GIT_TAG master
)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG develop
)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG master
)

FetchContent_MakeAvailable(lua sol2 yaml-cpp)

# Lua and sol2 are now linked in core/CMakeLists.txt


# Install configuration files
install(FILES game.cfg
    DESTINATION share/CHEngine
)

# Package configuration (simplified)
if(NOT FAST_CONFIG)
    set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
endif()



