# SHARED for local development (hot-reload), STATIC for CI/distribution/stability
option(CH_ENGINE_SHARED "Build engine as shared library (DLL)" OFF)
if(CH_ENGINE_SHARED)
    set(ENGINE_LIB_TYPE SHARED)
else()
    set(ENGINE_LIB_TYPE STATIC)
endif()

# We use STATIC libraries for sub-modules to ensure binary stability and avoid DLL ABI issues in MinGW
set(SUBMODULE_LIB_TYPE STATIC)

# If building as a shared library, all internal static sub-modules need to be aware
# they are part of a DLL build so they can dllexport their symbols.
if(CH_ENGINE_SHARED)
    add_compile_definitions(CH_DYNAMIC_LINK CH_ENGINE_BUILD)
endif()

add_subdirectory(common)
add_subdirectory(reflection)
add_subdirectory(imgui)
add_subdirectory(core)
add_subdirectory(assets)
add_subdirectory(graphics)
add_subdirectory(physics)
add_subdirectory(audio)
add_subdirectory(scene)
add_subdirectory(project)

add_subdirectory(platform)
#add_subdirectory(network)
add_subdirectory(${CMAKE_SOURCE_DIR}/scripting ${CMAKE_BINARY_DIR}/scripting)
add_subdirectory(app)
add_subdirectory(runtime)

# Create the final engine facade library that links everything together
add_library(engine ${ENGINE_LIB_TYPE} engine_dummy.cpp)

target_link_libraries(engine PUBLIC
    engine_app
)

target_compile_definitions(engine PRIVATE CH_ENGINE_BUILD)

if(CH_ENGINE_SHARED)
    target_compile_definitions(engine PUBLIC 
        CH_DYNAMIC_LINK
    )
endif()

# MSVC doesn't need WINDOWS_EXPORT_ALL_SYMBOLS anymore since we use CH_API
if(WIN32)
    set_target_properties(engine PROPERTIES PREFIX "")
endif()

target_include_directories(engine PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)


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

target_link_libraries(ChainedEngineFramework INTERFACE
    engine
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

