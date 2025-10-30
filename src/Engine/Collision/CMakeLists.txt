add_library(collision STATIC
    CollisionSystem.cpp
    CollisionSystem.h
    CollisionStructures.cpp
    CollisionStructures.h
    CollisionManager.cpp
    CollisionManager.h
    CollisionDebugRenderer.cpp
    CollisionDebugRenderer.h
)

# Disable Unity build for this library to avoid duplicate definitions
set_target_properties(collision PROPERTIES
    UNITY_BUILD OFF
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(collision PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)

target_link_libraries(collision PUBLIC raylib model)

# Add raylib include directories for FetchContent
if(raylib_POPULATED)
    target_include_directories(collision PUBLIC ${raylib_SOURCE_DIR}/src)
    message(STATUS "Collision module: Using raylib from FetchContent at ${raylib_SOURCE_DIR}/src")
elseif(NOT raylib_FOUND)
    message(WARNING "Collision module: raylib not found and not populated via FetchContent")
endif()

# Ensure raylib headers are available
if(TARGET raylib)
    get_target_property(raylib_INCLUDE_DIRS raylib INTERFACE_INCLUDE_DIRECTORIES)
    if(raylib_INCLUDE_DIRS)
        target_include_directories(collision PUBLIC ${raylib_INCLUDE_DIRS})
        message(STATUS "Collision module: Using raylib include dirs: ${raylib_INCLUDE_DIRS}")
    endif()
endif()