# Chained Engine - Dependencies
# Extracted from root CMakeLists.txt for modularity

include(FetchContent)

find_package(Threads REQUIRED)

# ============================================================================
# FetchContent Dependencies
# ============================================================================

# yaml-cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
)

# ImGuizmo (Manipulators)
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG master
)

FetchContent_MakeAvailable(yaml-cpp imguizmo)

# ============================================================================
# Raylib
# ============================================================================
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
        set(OPENGL_VERSION "4.3" CACHE STRING "" FORCE)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
        set(USE_EXTERNAL_GLFW IF_POSSIBLE CACHE STRING "" FORCE)
    endif()

    if(WIN32)
        set(OPENGL_VERSION "4.3" CACHE STRING "" FORCE)
    endif()

    add_subdirectory(include/raylib)
    message(STATUS "Raylib loaded from submodule")
    
    # Disable unity build for raylib to avoid GLAD header conflicts
    set_target_properties(raylib PROPERTIES UNITY_BUILD OFF)
else()
    message(FATAL_ERROR "Raylib submodule not found. Run: git submodule update --init --recursive")
endif()

# ============================================================================
# GoogleTest (for unit tests)
# ============================================================================
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

# ============================================================================
# EnTT (header-only library)
# ============================================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/include/entt/src")
    message(STATUS "Loading EnTT from submodule...")
    add_library(EnTT INTERFACE)
    target_include_directories(EnTT INTERFACE ${CMAKE_SOURCE_DIR}/include/entt/src)
    add_library(EnTT::EnTT ALIAS EnTT)
    message(STATUS "EnTT loaded from submodule (header-only)")
else()
    message(FATAL_ERROR "EnTT submodule not found. Run: git submodule update --init --recursive")
endif()

# ============================================================================
# ImGui + rlImGui + ImGuizmo
# ============================================================================
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
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    ${imguizmo_SOURCE_DIR}/ImGuizmo.h
)

add_library(imguilib STATIC ${IMGUI_SOURCES})

target_include_directories(imguilib PUBLIC
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/rlImGui
    ${imguizmo_SOURCE_DIR}
)
target_link_libraries(imguilib PRIVATE raylib)

# Define IMGUI math operators before including imgui.h
target_compile_definitions(imguilib PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# Disable unity build for imguilib to avoid GLAD header conflicts
set_target_properties(imguilib PROPERTIES UNITY_BUILD OFF)

# ============================================================================
# Native File Dialog (nfd)
# ============================================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/include/nfd/CMakeLists.txt")
    message(STATUS "Loading nfd from submodule...")
    set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(include/nfd)
    message(STATUS "nfd loaded from submodule")
endif()
