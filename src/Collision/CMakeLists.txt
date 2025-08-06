add_library(collision STATIC
    CollisionSystem.cpp
    CollisionSystem.h
    CollisionManager.cpp
    CollisionManager.h
)
target_link_libraries(collision PUBLIC raylib)