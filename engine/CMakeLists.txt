# Common include directories for all engine targets
set(ENGINE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
        .
    ${glm_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/coral/Coral.Native/Include
    ${CMAKE_SOURCE_DIR}/include/miniaudio
    ${CMAKE_SOURCE_DIR}/include/glfw/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/imguizmo
    ${CMAKE_SOURCE_DIR}/include/nfd/src/include
    ${CMAKE_BINARY_DIR}/gladsources/glad/include
    ${assimp_SOURCE_DIR}/include
    ${assimp_BINARY_DIR}/include
)

# SHARED for local development (hot-reload), STATIC for CI/distribution
set(CH_ENGINE_SHARED ON CACHE BOOL "Build engine as shared library (DLL)" FORCE)
if(CH_ENGINE_SHARED)
    set(ENGINE_LIB_TYPE SHARED)
else()
    set(ENGINE_LIB_TYPE STATIC)
endif()

# We use OBJECT libraries for sub-modules to handle circular dependencies (e.g. Core <-> Scene)
# Unified through Unity Build and PCH for maximum compilation speed.
set(SUBMODULE_LIB_TYPE OBJECT)

add_subdirectory(core)
add_subdirectory(graphics)
add_subdirectory(physics)
add_subdirectory(audio)
add_subdirectory(scene)
add_subdirectory(${CMAKE_SOURCE_DIR}/scripting ${CMAKE_BINARY_DIR}/scripting)

# Create the final engine facade library that links all sub-modules together
add_library(engine ${ENGINE_LIB_TYPE}
    $<TARGET_OBJECTS:engine_core>
    $<TARGET_OBJECTS:engine_graphics>
    $<TARGET_OBJECTS:engine_physics>
    $<TARGET_OBJECTS:engine_audio>
    $<TARGET_OBJECTS:engine_scene>
    $<TARGET_OBJECTS:engine_scripting>
)

target_compile_definitions(engine PRIVATE CH_ENGINE_BUILD)
    target_compile_definitions(engine PUBLIC 
        CH_DYNAMIC_LINK
        IMGUI_DEFINE_MATH_OPERATORS
        GLFW_INCLUDE_NONE
        IMGUI_IMPL_OPENGL_LOADER_GLAD
    )

# MSVC needs exported symbols to generate the .lib import library
set_target_properties(engine PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(WIN32)
    set_target_properties(engine PROPERTIES PREFIX "")
endif()

target_include_directories(engine PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${assimp_SOURCE_DIR}/include
    ${assimp_BINARY_DIR}/include
    ${glm_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
INTERFACE
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${imguizmo_SOURCE_DIR}
)

target_link_libraries(engine 
PUBLIC 
    EnTT::EnTT
    yaml-cpp
    assimp::assimp
PRIVATE
    imguilib
    nfd
    Coral.Native
)

# (Redundant compiler flags removed - now in CompilerSettings.cmake)

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine)
endif()

# ── Unified Framework Interface ──────────────────────────────────────────────
# This provides a single target for games to link against, encapsulating all
# engine headers and dependencies in a clean way.
add_library(ChainedEngineFramework INTERFACE)
add_library(ChainedEngine::Framework ALIAS ChainedEngineFramework)
add_library(ChainedEngine::API ALIAS ChainedEngineFramework)

target_include_directories(ChainedEngineFramework INTERFACE
    ${ENGINE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ChainedEngineFramework INTERFACE
    engine
    yaml-cpp
    assimp::assimp
)

install(TARGETS engine
    EXPORT ChainedEngineTargets
    ARCHIVE DESTINATION lib COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources"
    DESTINATION .
    USE_SOURCE_PERMISSIONS
    COMPONENT Runtime
)

