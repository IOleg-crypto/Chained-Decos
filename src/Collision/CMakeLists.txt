add_library(collision STATIC
    CollisionSystem.cpp
    CollisionSystem.h
)
target_link_libraries(collision PUBLIC raylib)