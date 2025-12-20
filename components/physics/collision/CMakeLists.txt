set(COLLISION_SOURCES
    core/collisionManager.cpp
    core/collisionManager.h
    interfaces/ICollisionManager.h
    system/collisionSystem.cpp
    system/collisionSystem.h
    structures/collisionStructures.cpp
    structures/collisionStructures.h
    structures/collisionComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/collisionDebugRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/collisionDebugRenderer.h
)

add_library(collision STATIC ${COLLISION_SOURCES})

# Disable Unity build for this library to avoid duplicate definitions
set_target_properties(collision PROPERTIES
    UNITY_BUILD OFF
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(collision PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Engine
)

target_link_libraries(collision PUBLIC raylib model EnTT::EnTT)

# Raylib is always available via submodule
if(TARGET raylib)
    get_target_property(raylib_INCLUDE_DIRS raylib INTERFACE_INCLUDE_DIRECTORIES)
    if(raylib_INCLUDE_DIRS)
        target_include_directories(collision PUBLIC ${raylib_INCLUDE_DIRS})
    endif()
endif()

