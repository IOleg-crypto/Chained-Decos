add_library(world STATIC
    Physics.cpp
    Physics.h
)

target_include_directories(world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)

target_link_libraries(world PUBLIC
    raylib
)
