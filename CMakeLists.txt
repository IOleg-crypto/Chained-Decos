cmake_minimum_required(VERSION 3.25)
project(ChainedDecos VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build configuration options
option(FAST_CONFIG "Reduce CMake configure time" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_MAP_EDITOR "Build MapEditor tools" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(ENABLE_PROFILING "Enable profiling support" OFF)
option(ENABLE_OPTIMIZATIONS "Enable compiler optimizations" ON)
option(ENABLE_WARNINGS "Enable strict compiler warnings" OFF)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(DISABLE_ALL_WARNINGS "Disable all compiler warnings" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_UNITY_BUILD "Enable unity build for faster compilation" ON)

# Platform-specific options
option(ENABLE_OPENGL_DEBUG "Enable OpenGL debug context" OFF)
option(ENABLE_AUDIO "Enable audio support" ON)
option(ENABLE_MULTITHREADING "Enable multithreading support" ON)

# Development options
option(ENABLE_DEV_TOOLS "Enable development tools" OFF)
option(ENABLE_DEBUG_INFO "Enable debug information" ON)
option(ENABLE_SANITIZERS "Enable sanitizers for debugging" OFF)

if(FAST_CONFIG)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.deps" CACHE PATH "Deps cache")
    set(FETCHCONTENT_QUIET ON)
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)
endif()

include(FetchContent)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    if(ENABLE_OPTIMIZATIONS)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    else()
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    endif()
endif()

# Set install prefix for CI/CD environments
if(DEFINED ENV{CI})
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install prefix" FORCE)
else()
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install prefix" FORCE)
endif()

# Compiler-specific settings
if(MSVC)
    # MSVC-specific settings
    add_compile_options(
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/DNDEBUG>
    )

    if(DISABLE_ALL_WARNINGS)
        add_compile_options(/W0)
    elseif(ENABLE_WARNINGS)
        add_compile_options(/W4 /permissive-)
        if(ENABLE_DEBUG_INFO)
            add_compile_options(/Zi)
        endif()
    endif()

    # Set consistent CRT settings for all targets
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/MDd)  # MultiThreadedDebugDLL
    else()
        add_compile_options(/MD)   # MultiThreadedDLL
    endif()

    if(ENABLE_SANITIZERS)
        add_compile_options(/fsanitize=address)
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang settings
    add_compile_options(
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-DNDEBUG>
    )

    if(DISABLE_ALL_WARNINGS)
        add_compile_options(-w)
    elseif(ENABLE_WARNINGS)
        add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-Wmost -Wno-missing-braces -Wno-missing-field-initializers -Wno-attributes)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-Wno-missing-field-initializers -Wno-attributes)
        endif()

        if(WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
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

    # Ensure proper DLL loading on Windows
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7+

    # Avoid conflicts between raylib and Windows API - more targeted approach
    # Exclude specific parts of Windows API that conflict with raylib
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
endif()

if(APPLE)
    add_compile_definitions(GL_SILENCE_DEPRECATION)
endif()

if(ENABLE_MULTITHREADING)
    find_package(Threads REQUIRED)
endif()

# Unity build configuration
if(ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)
endif()

# --- Fetch external dependencies ---
set(RAYLIB_VERSION "5.5")
set(NLOHMANN_JSON_VERSION "3.12.0")

# Raylib
find_package(raylib ${RAYLIB_VERSION} CONFIG QUIET)

if(NOT raylib_FOUND)
    message(STATUS "Raylib ${RAYLIB_VERSION} not found, fetching...")
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    # Raylib build configuration
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

    # Platform-specific raylib options
    if(UNIX AND NOT APPLE)
        set(USE_WAYLAND OFF CACHE BOOL "" FORCE)  # Use X11 on Linux
        set(PLATFORM Desktop CACHE STRING "" FORCE)  # Use Desktop platform (GLFW) for better compatibility
        set(OPENGL_VERSION "3.3" CACHE STRING "" FORCE)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)  # Ensure X11 support in GLFW
    endif()

    FetchContent_MakeAvailable(raylib)

    # Ensure raylib include directories are available for all targets
    if(raylib_POPULATED)
        message(STATUS "Raylib fetched successfully, setting up include directories...")
        set(raylib_INCLUDE_DIRS "${raylib_SOURCE_DIR}/src")
        message(STATUS "Raylib include dirs: ${raylib_INCLUDE_DIRS}")
    endif()
endif()

# Ensure raylib is available even if found via find_package
if(raylib_FOUND OR raylib_POPULATED)
    message(STATUS "Raylib is available")
    if(raylib_POPULATED)
        # For FetchContent, ensure include directories are set
        set_target_properties(raylib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${raylib_SOURCE_DIR}/src")
    endif()
endif()

# Add winmm library for Windows timer functions (needed by raylib)
if(WIN32)
    link_libraries(winmm)
endif()

# nlohmann_json
find_package(nlohmann_json CONFIG QUIET)

if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching...")
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/tags/v${NLOHMANN_JSON_VERSION}.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# GoogleTest (for unit tests)
if(BUILD_TESTS)
    find_package(GTest CONFIG QUIET)

    if(NOT GTest_FOUND)
        message(STATUS "GoogleTest not found, fetching...")
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        # Use dynamic CRT for better Windows compatibility and consistent debug levels
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        # Force GoogleTest to use the same CRT settings as the main project
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_compile_definitions(_DEBUG)
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        else()
            add_compile_definitions(NDEBUG)
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        endif()
        FetchContent_MakeAvailable(googletest)
    endif()
endif()


add_library(imguilib STATIC
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

add_subdirectory(src/Engine)
add_subdirectory(src/Game)

add_executable(ChainedDecos src/main.cpp)
target_link_libraries(ChainedDecos PRIVATE
    ChainedDecosGame
    imguilib
)

if(BUILD_TESTS)
    add_subdirectory(tests ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()

if(BUILD_MAP_EDITOR)
    add_subdirectory(src/Game/MapEditor)
endif()

# Set the target properties
set_target_properties(ChainedDecos PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    POSITION_INDEPENDENT_CODE ON
)

# Set output name based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(ChainedDecos PROPERTIES OUTPUT_NAME "ChainedDecos_debug")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(ChainedDecos PROPERTIES OUTPUT_NAME "ChainedDecos")
endif()

# Enable benchmarks if requested
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation configuration
install(TARGETS ChainedDecos
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# # Install resources (only if directory exists)
# if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
#     install(DIRECTORY resources/
#         DESTINATION share/ChainedDecos/resources
#         FILES_MATCHING PATTERN "*"
#     )
# endif()

# Install configuration files
install(FILES game.cfg
    DESTINATION share/ChainedDecos
)

# Package configuration
if(NOT FAST_CONFIG)
    set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
    export(TARGETS ChainedDecos
        NAMESPACE ChainedDecos::
        FILE ChainedDecosConfig.cmake
    )

    # Generate package version file
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ChainedDecosConfigVersion.cmake"
        VERSION 1.0.0
        COMPATIBILITY AnyNewerVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ChainedDecosConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ChainedDecosConfigVersion.cmake"
        DESTINATION lib/cmake/ChainedDecos
    )
endif()

# CPack configuration for packaging
if(NOT FAST_CONFIG)
    set(CPACK_PACKAGE_NAME "ChainedDecos")
    set(CPACK_PACKAGE_VERSION "1.0.0")
    set(CPACK_PACKAGE_DESCRIPTION "Chained Decos - 3D Parkour Game")
    set(CPACK_PACKAGE_VENDOR "Chained Decos Team")
    set(CPACK_PACKAGE_CONTACT "contact@chaineddecos.com")

    if(WIN32)
        set(CPACK_GENERATOR "ZIP;NSIS")
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows")
    elseif(APPLE)
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macos")
    else()
        set(CPACK_GENERATOR "TGZ;DEB")
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux")
    endif()

    include(CPack)
endif()
