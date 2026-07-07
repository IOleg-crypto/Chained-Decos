# Create a high-level runtime module that owns the Application logic and initialization
# This module depends on all other engine modules, but modules don't depend on it.
chained_add_engine_module(engine_app
    SOURCES
        application.cpp
        application.h
        application_types.h
        entry_point.h
    DEPENDS
        engine_scripting
        engine_scene
        engine_project
        engine_graphics
        engine_physics
        engine_audio
        engine_assets
        engine_core
        engine_imgui
        engine_platform
)

target_include_directories(engine_app PRIVATE ${CMAKE_SOURCE_DIR})
