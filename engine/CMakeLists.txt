
# Common include directories for all engine targets
set(ENGINE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${glm_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/entt/src
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
    raylib 
    EnTT
    imguilib
    yaml-cpp
PRIVATE
    assimp::assimp
)

# Compiler flags
if(MSVC)
    target_compile_options(engine PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine PRIVATE -Wa,-mbig-obj)
endif()

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine)
endif()

install(TARGETS engine
    EXPORT ChainedEngineTargets
    ARCHIVE DESTINATION lib COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    DESTINATION engine
    USE_SOURCE_PERMISSIONS
    COMPONENT Runtime
)
