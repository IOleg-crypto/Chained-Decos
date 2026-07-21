chained_add_engine_module(engine_physics
    SOURCES
        physics.cpp
        jolt_physics_world.cpp
        physics.h
        raycast_result.h
        iphysics_world.h
        physics_types.h
        jolt_physics_world.h
    DEPENDS
        engine_core
        engine_project
        engine_external_entt
        glm
        assimp
        Jolt
)

if(MSVC)
    target_compile_options(engine_physics PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_physics PRIVATE -Wa,-mbig-obj)
endif()
