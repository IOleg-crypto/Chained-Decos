add_library(physics STATIC
    PhysicsComponent.cpp
    PhysicsComponent.h
    SurfaceComponent.h
    SurfaceComponent.cpp
)

target_include_directories(physics
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(physics
    PUBLIC
    raylib
)
