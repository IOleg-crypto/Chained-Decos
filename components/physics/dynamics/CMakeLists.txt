set(DYNAMICS_SOURCES
    components/physics_component.cpp
    components/physics_component.h
    components/surface_component.h
    components/surface_component.cpp
)

add_library(physics STATIC ${DYNAMICS_SOURCES})
target_include_directories(physics
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
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


