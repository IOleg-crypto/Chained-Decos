add_library(engine_physics OBJECT
    physics.cpp
    dynamics.cpp
    narrow_phase.cpp
    scene_trace.cpp
    bvh/bvh.cpp

    physics.h
    dynamics.h
    narrow_phase.h
    scene_trace.h
    bvh/bvh.h
    bvh/bvh_node.h
    collision/collision.h
    collision/collision_triangle.h
)

target_include_directories(engine_physics PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${assimp_SOURCE_DIR}/include
    ${assimp_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/entt/src
)

target_link_libraries(engine_physics PUBLIC 
    raylib 
    EnTT
)

if(MSVC)
    target_compile_options(engine_physics PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_physics PRIVATE -Wa,-mbig-obj)
endif()

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_physics)
endif()
