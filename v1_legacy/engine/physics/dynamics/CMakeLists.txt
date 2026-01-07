set(DYNAMICS_SOURCES
    components/physics_component.cpp
    components/physics_component.h
    components/surface_component.h
    components/surface_component.cpp
)

add_library(physics_core STATIC ${DYNAMICS_SOURCES})
target_include_directories(physics_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(physics_core
    PUBLIC
    raylib
)

set_target_properties(physics_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Add raylib include directories for FetchContent
if(NOT raylib_FOUND)
    target_include_directories(physics_core PUBLIC ${raylib_SOURCE_DIR}/src)
endif()


