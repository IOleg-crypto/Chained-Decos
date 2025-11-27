add_library(ChainedDecosGame STATIC
    GameApplication.cpp
    GameApplication.h
    Managers/MapCollisionInitializer.cpp
    Managers/MapCollisionInitializer.h
    # Systems (modular architecture)
    Systems/PlayerSystem/PlayerSystem.cpp
    Systems/PlayerSystem/PlayerSystem.h
    Systems/MapSystem/MapSystem.cpp
    Systems/MapSystem/MapSystem.h
    Systems/UIController/UIController.cpp
    Systems/UIController/UIController.h
    Systems/RenderingSystem/RenderingSystem.cpp
    Systems/RenderingSystem/RenderingSystem.h
)

target_include_directories(ChainedDecosGame
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/project/chained_decos
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/project/chained_decos/Player
        ${CMAKE_SOURCE_DIR}/servers/physics/collision
        ${CMAKE_SOURCE_DIR}/scene/resources/model
        ${CMAKE_SOURCE_DIR}/scene/resources/map
        ${CMAKE_SOURCE_DIR}/servers/rendering
        ${CMAKE_SOURCE_DIR}/include/imgui
        ${CMAKE_SOURCE_DIR}/include/rlImGui
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/scene/3d/camera
        ${CMAKE_SOURCE_DIR}/scene/main
        ${CMAKE_SOURCE_DIR}/servers/physics/dynamics/src
)

# Add PROJECT_ROOT_DIR definition for consistent path handling
target_compile_definitions(ChainedDecosGame PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Use forward slashes for cross-platform compatibility
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_DIR_ESCAPED)
target_compile_definitions(ChainedDecosGame PRIVATE
    PROJECT_ROOT_DIR="${PROJECT_ROOT_DIR_ESCAPED}"
)

add_subdirectory(Menu)
add_subdirectory(Player)

target_link_libraries(ChainedDecosGame
    PUBLIC
        ChainedDecosEngine
        player
        menu
        raylib
        imguilib
        nlohmann_json
)
