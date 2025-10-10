add_library(mapmod STATIC
    MapLoader.cpp
    MapLoader.h
    ParkourMapGenerator.cpp
    ParkourMapGenerator.h
    ../MapEditor/MapFileManager/JsonMapFileManager.cpp
    ../MapEditor/MapFileManager/JsonMapFileManager.h
)

target_include_directories(mapmod
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(mapmod PUBLIC raylib nlohmann_json)