add_library(world STATIC
    Core/World.cpp
    Core/World.h
)

target_include_directories(world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(world PUBLIC
    raylib
    physics)

set_target_properties(world PROPERTIES POSITION_INDEPENDENT_CODE ON)
