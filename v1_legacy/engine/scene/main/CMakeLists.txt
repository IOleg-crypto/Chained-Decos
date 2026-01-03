set(WORLD_SOURCES
    world.cpp
    world.h
)

add_library(world STATIC ${WORLD_SOURCES})

target_include_directories(world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(world PUBLIC
    raylib
    physics 
    core
    map
    render
    model
    ecs
)

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
target_compile_definitions(world PRIVATE
    PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}"
)

set_target_properties(world PROPERTIES POSITION_INDEPENDENT_CODE ON)
