
# Chained Engine - Dependencies
# Extracted from root CMakeLists.txt for modularity

# yaml-cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_SOURCE_DIR}/include/yaml-cpp/CMakeLists.txt")
    add_subdirectory(include/yaml-cpp)
    set(yaml-cpp_SOURCE_DIR "${CMAKE_SOURCE_DIR}/include/yaml-cpp" CACHE INTERNAL "")
else()
    message(FATAL_ERROR "yaml-cpp submodule not found! Run: git submodule update --init --recursive")
endif()

# ImGuizmo (Manipulators)
set(imguizmo_SOURCE_DIR "${CMAKE_SOURCE_DIR}/include/imguizmo")

# GLM
if(EXISTS "${CMAKE_SOURCE_DIR}/include/glm/CMakeLists.txt")
    # GLM is header-only but provides CMake integration
    add_subdirectory(include/glm)
    set(glm_SOURCE_DIR "${CMAKE_SOURCE_DIR}/include/glm" CACHE INTERNAL "")
else()
    message(FATAL_ERROR "glm submodule not found! Run: git submodule update --init --recursive")
endif()

# Coral (for C# scripting integration)
if(EXISTS "${CMAKE_SOURCE_DIR}/include/coral/cmake/CMakeLists.txt")
    add_subdirectory(include/coral/cmake)
    set(coral_SOURCE_DIR "${CMAKE_SOURCE_DIR}/include/coral" CACHE INTERNAL "")
    
    # --- CI Fixes for Coral (Injection) ---
    if(WIN32)
        set(CORAL_FIX_DIR "${CMAKE_BINARY_DIR}/coral_fixes")
        file(MAKE_DIRECTORY "${CORAL_FIX_DIR}")
        
        # 1. ShlObj_core.h shim (MinGW fix)
        if(MINGW)
            file(WRITE "${CORAL_FIX_DIR}/ShlObj_core.h" "#pragma once\n#include <shlobj.h>\n")
        endif()
        
        # 2. MSVC wchar_t stream fix (C2280 fix)
        file(WRITE "${CORAL_FIX_DIR}/StreamFix.hpp" 
            "#pragma once\n"
            "#include <iostream>\n"
            "#include <string>\n"
            "// Standalone fix for deleted operator<< in Coral logging\n"
            "inline std::ostream& operator<<(std::ostream& os, const wchar_t* str) { return os << \"[wide string]\"; }\n"
            "inline std::ostream& operator<<(std::ostream& os, const std::wstring& str) { return os << \"[wide string]\"; }\n"
        )
        
        if(TARGET Coral.Native)
            if(MINGW)
                target_include_directories(Coral.Native PRIVATE "${CORAL_FIX_DIR}")
            endif()
            
            if(MSVC)
                target_compile_options(Coral.Native PRIVATE "/FI${CORAL_FIX_DIR}/StreamFix.hpp")
            else()
                target_compile_options(Coral.Native PRIVATE "-include${CORAL_FIX_DIR}/StreamFix.hpp")
            endif()
        endif()
    endif()
else()
    message(FATAL_ERROR "coral submodule not found! Run: git submodule update --init --recursive")
endif()

# assimp (Asset Importer Library)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)

# Enable ALL model format importers to ensure maximum compatibility
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT ON CACHE BOOL "" FORCE)
# Specific overrides if needed (currently all on by default)

# Ensure no unity build for assimp or its subprojects
set(CMAKE_UNITY_BUILD OFF) 

if(EXISTS "${CMAKE_SOURCE_DIR}/include/assimp/CMakeLists.txt")
    add_subdirectory(include/assimp)
    set(assimp_SOURCE_DIR "${CMAKE_SOURCE_DIR}/include/assimp" CACHE INTERNAL "")
    set(assimp_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/include/assimp" CACHE INTERNAL "")
    # Disable unity build for assimp to avoid header/namespace conflicts
    if(TARGET assimp)
        set_target_properties(assimp PROPERTIES UNITY_BUILD OFF)
        if(NOT MSVC)
            target_compile_options(assimp PRIVATE -Wno-error)
        endif()
    endif()
else()
    message(FATAL_ERROR "assimp submodule not found! Run: git submodule update --init --recursive")
endif()

# ============================================================================
# GLFW (Standalone)
# ============================================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/include/glfw/CMakeLists.txt")
    message(STATUS "Loading standalone GLFW...")
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(include/glfw)

    if(UNIX AND NOT APPLE)
        find_package(X11 REQUIRED)
        target_link_libraries(glfw PUBLIC ${X11_LIBRARIES})
    endif()

    set(GLFW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/include/glfw" CACHE INTERNAL "")
else()
    message(FATAL_ERROR "Standalone GLFW not found in include/glfw")
endif()

# ============================================================================
# GLAD (Standalone Generator)
# ============================================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/include/glad/cmake/CMakeLists.txt")
    message(STATUS "Loading standalone GLAD generator...")
    # Add the glad subdirectory which provides glad_add_library
    add_subdirectory(include/glad/cmake EXCLUDE_FROM_ALL)
    
    # Generate GLAD for OpenGL 4.3 Core
    glad_add_library(glad STATIC API gl:core=4.3)
else()
    message(FATAL_ERROR "Standalone GLAD generator not found in include/glad")
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
    if(NOT TARGET EnTT::EnTT)
        add_library(EnTT::EnTT ALIAS EnTT)
    endif()
    message(STATUS "EnTT loaded from submodule (header-only)")
else()
    message(FATAL_ERROR "EnTT submodule not found. Run: git submodule update --init --recursive")
endif()

# ============================================================================
# ImGui Standalone (GLFW + OpenGL3)
# ============================================================================
set(IMGUI_SOURCES
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/misc/cpp/imgui_stdlib.cpp
    include/imgui/backends/imgui_impl_glfw.cpp
    include/imgui/backends/imgui_impl_glfw.h
    include/imgui/backends/imgui_impl_opengl3.cpp
    include/imgui/backends/imgui_impl_opengl3.h
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    ${imguizmo_SOURCE_DIR}/ImGuizmo.h
)

if(NOT TARGET imguilib)
    add_library(imguilib STATIC ${IMGUI_SOURCES})

    target_include_directories(imguilib PUBLIC
        ${CMAKE_SOURCE_DIR}/include/imgui
        ${CMAKE_SOURCE_DIR}/include/glfw/include
        ${imguizmo_SOURCE_DIR}
    )
    target_link_libraries(imguilib PUBLIC glfw glad)

    # Define IMGUI math operators and GLFW settings
    target_compile_definitions(imguilib PUBLIC 
        IMGUI_DEFINE_MATH_OPERATORS
        GLFW_INCLUDE_NONE
        IMGUI_IMPL_OPENGL_LOADER_GLAD
    )

    # Disable unity build for imguilib to avoid GLAD header conflicts
    set_target_properties(imguilib PROPERTIES UNITY_BUILD OFF)
endif()

# ============================================================================
# Native File Dialog (nfd)
# ============================================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/include/nfd/CMakeLists.txt")
    message(STATUS "Loading nfd from submodule...")
    set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(include/nfd)
    message(STATUS "nfd loaded from submodule")
endif()
