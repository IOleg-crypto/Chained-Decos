chained_add_engine_module(engine_physics
    SOURCES
        physics.cpp
        dynamics.cpp
        collision_core.cpp
        raycast_query.cpp
        default_physics_world.cpp
        bvh/bvh_cache.cpp
        bvh/bvh.cpp
        collision/collision_triangle.cpp
        physics.h
        dynamics.h
        collision_core.h
        raycast_query.h
        iphysics_world.h
        default_physics_world.h
        bvh/bvh_cache.h
        bvh/bvh.h
        bvh/bvh_node.h
        collision/collision.h
        collision/collision_triangle.h
        raycast_result.h
        jolt_physics_world.h
        jolt_physics_world.cpp
    DEPENDS
        engine_core
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
