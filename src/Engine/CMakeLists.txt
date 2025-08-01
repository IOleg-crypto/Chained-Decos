add_library(engine STATIC
    Engine.cpp
    Engine.h
)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine)
target_link_libraries(engine PUBLIC raylib nlohmann_json imguilib) 