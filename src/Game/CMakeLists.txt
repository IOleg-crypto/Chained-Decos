
file(GLOB_RECURSE GAME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(ChainedDecosGame ${GAME_SOURCES})

target_include_directories(ChainedDecosGame PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ChainedDecosGame
    ChainedDecosEngine
    raylib
    imguilib
    nlohmann_json::nlohmann_json
)
