add_library(player STATIC
    Player.cpp
    Player.h
    PlayerCollision.cpp
    PlayerCollision.h
    PlayerInput.cpp
    PlayerInput.h
    PlayerModel.h
    PlayerModel.cpp
    PlayerMovement.h
    PlayerMovement.cpp

)
target_link_libraries(player PUBLIC raylib nlohmann_json collision world)