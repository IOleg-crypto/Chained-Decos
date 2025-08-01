add_library(mapmod STATIC
    MapLoader.cpp
    MapLoader.h
)
target_include_directories(mapmod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Map)
target_link_libraries(mapmod PUBLIC raylib) 