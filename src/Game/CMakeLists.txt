add_library(ChainedDecosGame STATIC
    GameApplication.cpp
    GameApplication.h
    Managers/MapManager.cpp
    Managers/MapManager.h
    Managers/ResourceManager.cpp
    Managers/ResourceManager.h
    Managers/StateManager.cpp
    Managers/StateManager.h
    Managers/GameRenderHelpers.cpp
    Managers/GameRenderHelpers.h
    Managers/PlayerManager.cpp
    Managers/PlayerManager.h
    Managers/UpdateManager.cpp
    Managers/UpdateManager.h
    Managers/MenuActionHandler.cpp
    Managers/MenuActionHandler.h
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
        ${CMAKE_SOURCE_DIR}/include
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
        module
)
