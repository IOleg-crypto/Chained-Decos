add_library(engine_physics ${SUBMODULE_LIB_TYPE}
        physics.cpp
        dynamics.cpp
    collision_core.cpp
    raycast_query.cpp
    default_physics_world.cpp
    physics_system.cpp
        bvh/bvh_cache.cpp
        bvh/bvh.cpp
        collision/collision_triangle.cpp

        physics.h
        dynamics.h
    collision_core.h
    raycast_query.h
    iphysics_world.h
    default_physics_world.h
    physics_system.h
        bvh/bvh_cache.h
        bvh/bvh.h
        bvh/bvh_node.h
        collision/collision.h
        collision/collision_triangle.h
        raycast_result.h
)

target_include_directories(engine_physics PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${assimp_SOURCE_DIR}/include
    ${assimp_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/glfw/include
)

target_link_libraries(engine_physics PUBLIC 
    engine_core
    EnTT::EnTT
)

if(MSVC)
    target_compile_options(engine_physics PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_physics PRIVATE -Wa,-mbig-obj)
endif()

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_physics)
endif()
