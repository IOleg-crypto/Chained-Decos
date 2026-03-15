
# Common include directories for all engine targets
set(ENGINE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${glm_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/coral/Coral.Native/Include
    ${CMAKE_SOURCE_DIR}/include/raylib/src
)

# SHARED for local development (hot-reload), STATIC for CI/distribution
option(CH_ENGINE_SHARED "Build engine as shared library (DLL)" ON)
if(CH_ENGINE_SHARED)
    set(ENGINE_LIB_TYPE SHARED)
else()
    set(ENGINE_LIB_TYPE STATIC)
endif()

add_subdirectory(core)
add_subdirectory(graphics)
add_subdirectory(physics)
add_subdirectory(audio)
add_subdirectory(scene)

# Create the final engine facade library that stitches all objects together
add_library(engine ${ENGINE_LIB_TYPE}
    $<TARGET_OBJECTS:engine_core>
    $<TARGET_OBJECTS:engine_graphics>
    $<TARGET_OBJECTS:engine_physics>
    $<TARGET_OBJECTS:engine_audio>
    $<TARGET_OBJECTS:engine_scene>
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
    EnTT
PRIVATE
    raylib 
    imguilib
    yaml-cpp
    assimp::assimp
)

# (Redundant compiler flags removed - now in CompilerSettings.cmake)

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine)
endif()

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

