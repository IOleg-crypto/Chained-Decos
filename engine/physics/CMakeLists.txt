# Register Physics sources to the unified engine target
target_sources(engine PRIVATE
    # Physics Core
    ${CMAKE_CURRENT_SOURCE_DIR}/physics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dynamics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dynamics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/narrow_phase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/narrow_phase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_trace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_trace.h

    # Collision
    ${CMAKE_CURRENT_SOURCE_DIR}/collision/collision.h
    ${CMAKE_CURRENT_SOURCE_DIR}/collision/collision_triangle.h
    
    # BVH
    ${CMAKE_CURRENT_SOURCE_DIR}/bvh/bvh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bvh/bvh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bvh/bvh_node.h
)
