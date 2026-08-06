add_library(engine_physics STATIC
    physics.h
    physics.cpp
    iphysics_world.h
    physics_types.h
    raycast_result.h
    jolt_physics_world.h
    jolt_physics_world.cpp
)

target_include_directories(engine_physics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_physics PUBLIC
    engine_core
    engine_project
    engine_external_entt
    glm
    assimp
    Jolt
)

if(TARGET engine_pch)
    target_link_libraries(engine_physics PRIVATE engine_pch)
endif()
