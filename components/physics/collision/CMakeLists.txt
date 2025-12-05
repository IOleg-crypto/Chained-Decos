add_library(collision STATIC
    Core/CollisionManager.cpp
    Core/CollisionManager.h
    Interfaces/ICollisionManager.h
    System/CollisionSystem.cpp
    System/CollisionSystem.h
    Structures/CollisionStructures.cpp
    Structures/CollisionStructures.h
    Structures/CollisionComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Debug/CollisionDebugRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Debug/CollisionDebugRenderer.h
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

# Raylib is always available via submodule
if(TARGET raylib)
    get_target_property(raylib_INCLUDE_DIRS raylib INTERFACE_INCLUDE_DIRECTORIES)
    if(raylib_INCLUDE_DIRS)
        target_include_directories(collision PUBLIC ${raylib_INCLUDE_DIRS})
    endif()
endif()