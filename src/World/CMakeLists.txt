add_library(world STATIC
    PhysicsData.h
)
target_include_directories(world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/World)