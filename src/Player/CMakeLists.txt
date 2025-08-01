add_library(player STATIC
    Player.cpp
    Player.h
    PositionData.h
)
target_include_directories(player PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Player)
target_link_libraries(player PUBLIC raylib) 