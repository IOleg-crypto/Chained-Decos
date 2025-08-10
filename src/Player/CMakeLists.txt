add_library(player STATIC
    Player.cpp
    Player.h
)
target_link_libraries(player PUBLIC raylib nlohmann_json collision world)