add_library(engine STATIC
    Engine.cpp
    Engine.h
)

target_link_libraries(engine PUBLIC raylib nlohmann_json imguilib) 