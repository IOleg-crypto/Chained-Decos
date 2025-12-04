add_library(physics STATIC
    Components/PhysicsComponent.cpp
    Components/PhysicsComponent.h
    Components/SurfaceComponent.h
    Components/SurfaceComponent.cpp
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

set_target_properties(physics PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Add raylib include directories for FetchContent
if(NOT raylib_FOUND)
    target_include_directories(physics PUBLIC ${raylib_SOURCE_DIR}/src)
endif()
