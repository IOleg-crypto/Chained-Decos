add_library(collision STATIC
    CollisionSystem.cpp
    CollisionSystem.h
    CollisionStructures.cpp
    CollisionStructures.h
    CollisionManager.cpp
    CollisionManager.h
    CollisionDebugRenderer.cpp
    CollisionDebugRenderer.h
    Octree.cpp
    Octree.h
    SmartCollisionSystem.cpp
    SmartCollisionSystem.h
)
target_link_libraries(collision PUBLIC raylib)