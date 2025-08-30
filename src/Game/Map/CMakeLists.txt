add_library(mapmod STATIC
    MapLoader.cpp
    MapLoader.h
)
target_link_libraries(mapmod PUBLIC raylib nlohmann_json) 