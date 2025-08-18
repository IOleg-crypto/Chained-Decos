add_library(collision STATIC
    CollisionSystem.cpp
    CollisionSystem.h
    CollisionStructures.cpp
    CollisionStructures.h
    CollisionManager.cpp
    CollisionManager.h
    CollisionDebugRenderer.cpp
    CollisionDebugRenderer.h
    CollisionUtils.h
    Octree.cpp
    Octree.h
)
target_link_libraries(collision PUBLIC raylib)