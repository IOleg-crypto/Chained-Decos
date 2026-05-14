# SHARED for local development (hot-reload), STATIC for CI/distribution/stability
option(CH_ENGINE_SHARED "Build engine as shared library (DLL)" OFF)
if(CH_ENGINE_SHARED)
    set(ENGINE_LIB_TYPE SHARED)
else()
    set(ENGINE_LIB_TYPE STATIC)
endif()

# We use STATIC libraries for sub-modules to ensure binary stability and avoid DLL ABI issues in MinGW
set(SUBMODULE_LIB_TYPE STATIC)

add_subdirectory(core)
add_subdirectory(assets)
add_subdirectory(graphics)
add_subdirectory(physics)
add_subdirectory(audio)
add_subdirectory(scene)
add_subdirectory(${CMAKE_SOURCE_DIR}/scripting ${CMAKE_BINARY_DIR}/scripting)

# Create the final engine facade library that links all sub-modules together
add_library(engine ${ENGINE_LIB_TYPE} core/application.cpp)

target_link_libraries(engine PUBLIC
    engine_scripting
    engine_scene
    engine_graphics
    engine_physics
    engine_audio
    engine_assets
    engine_core
    ChainedEngine::External::GNS
)

if (NOT CH_ENGINE_SHARED)
    # When building statically, we form a cycle so application.cpp can satisfy module requirements
    target_link_libraries(engine_scripting PUBLIC engine)
    target_link_libraries(engine_scene PUBLIC engine)
endif()

target_compile_definitions(engine PRIVATE CH_ENGINE_BUILD)
target_compile_definitions(engine PUBLIC 
    CH_DYNAMIC_LINK
)

# MSVC needs exported symbols to generate the .lib import library
set_target_properties(engine PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(WIN32)
    set_target_properties(engine PROPERTIES PREFIX "")
endif()

target_include_directories(engine PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
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

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)

target_include_directories(ChainedEngineFramework INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(ChainedEngineFramework INTERFACE
    engine
    ChainedEngine::External::GLM
    ChainedEngine::External::EnTT
    ChainedEngine::External::YamlCpp
)

target_compile_definitions(ChainedEngineFramework INTERFACE
    PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}"
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

