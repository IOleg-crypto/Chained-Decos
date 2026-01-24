set(GAME_SOURCES
    src/game_scripts.cpp
    src/PlayerController.h
    src/CameraController.h
)

add_library(ChainedDecosGame STATIC ${GAME_SOURCES})
target_link_libraries(ChainedDecosGame PUBLIC engine)
target_include_directories(ChainedDecosGame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
