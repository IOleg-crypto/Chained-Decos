add_library(ChainedDecosGame STATIC
    Game.cpp
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

add_subdirectory(Map)
add_subdirectory(Menu)
add_subdirectory(Player)

target_link_libraries(ChainedDecosGame
    PUBLIC
        ChainedDecosEngine
        player
        menu
        mapmod
        raylib
        imguilib
        nlohmann_json
)
