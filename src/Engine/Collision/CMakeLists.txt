add_library(collision STATIC
    CollisionSystem.cpp
    CollisionSystem.h
    CollisionStructures.cpp
    CollisionStructures.h
    CollisionManager.cpp
    CollisionManager.h
    CollisionDebugRenderer.cpp
    CollisionDebugRenderer.h
    GroundColliderFactory.cpp
    GroundColliderFactory.h
)

target_include_directories(collision PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)

target_link_libraries(collision PUBLIC raylib model)